#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "chibicc.h"

Token *token;

#define RED() "\x1b[31m"
#define NRM() "\x1b[0m"
static char *user_input;
static char *user_file = "<stdin>";
static char *get_line_col(int *line, int *col, int pos) {
	char *str = user_input;
	*line = 1;
	*col = 0;
	if (strcmp(user_file, "<stdin>")) {
		*col = 1;
		for (int i = 0; i < pos; i++) {
			if (user_input[i] == '\n') {
				*line+=1;
				*col = 0;
				str = &user_input[i + 1];
			}
			*col+=1;
		}
	}
	return str;
}

static void add_line_numbers(Token *tok) {
	char *p = user_input;
	int n = 1;
	do {
		if (!tok) {
			break;
		}
		if (p == tok->loc) {
			tok->line_no = n;
			tok = tok->next;
		}
		if (*p == '\n') {
			n++;
		}
	} while (*p++);
}

// Print error message in the following format:
// parse.c:529:funcall:test/quine.i:36:80: error: Implicit declaration of a function
// int main(){char*s="int main(){char*s=%c%s%c;printf(s,34,s,34,10);return 0;}%c";printf(s,34,s,34,10);return 0;}
//                                                                                ^~~~~~
static void verror_at(char *loc, int len, char *fmt, va_list ap) {
	int pos = 0;
	char *str = 0;
	if (loc) {
		pos = loc - user_input;
		int user_line, user_col;
		str = get_line_col(&user_line, &user_col, pos);
		fprintf(stderr, "%s:%d:%d: %serror%s: ", user_file, user_line, user_col, RED(), NRM());
		pos = user_col - 1;
	}
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, "\n");
	if (loc) {
		int slen = strlen(str);
		char *p = strchr(str, '\n');
		if (p) {
			slen = p - str;
		}
#if 0
		fprintf(stderr, "%.*s\n", slen, str);
#else
		fprintf(stderr, "%.*s", pos, str);
		fprintf(stderr, "%s%.*s%s", RED(), len, str+pos, NRM());
		fprintf(stderr, "%.*s\n", slen-pos-len, str+pos+len);
#endif
		fprintf(stderr, "%*s", pos, "");
#if 0
		fprintf(stderr, "^ len=%d\n", len);
#else
		fprintf(stderr, "^");
		for (int i = 1; i < len; i++) {
			fprintf(stderr, "~");
		}
		fprintf(stderr, "\n");
#endif
	}
}

void error(char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	verror_at(0, 0, fmt, ap);
	exit(1);
}

void error_at(char *loc, char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	verror_at(loc, 0, fmt, ap);
	exit(1);
}

void _error_tok(Token *tok, char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	verror_at(tok->loc, tok->len, fmt, ap);
	exit(1);
}

void print(char *s) {
	fprintf(stderr, "%s\n", s);
}

void print_str(char *s, Token *token) {
	char buf[1024];
	snprintf(buf, token->len + 1, "%s", token->loc);
	fprintf(stderr, "%s'%s'\n", s, buf);
}

void print_num(char *s, Token *token) {
	fprintf(stderr, "%s'%ld'\n", s, token->val);
}

void print_token(Token *token) {
	if (!token) {
		return;
	}
	switch (token->kind) {
		case TK_IDENT:
			print_str("IDN", token);
			break;
		case TK_PUNCT:
			print_str("PUN", token);
			break;
		case TK_NUM:
			print_num("NUM", token);
			break;
		case TK_KEYWORD:
			print_str("KEY", token);
			break;
		case TK_EOF:
			fprintf(stderr, "EOF\n");
			break;
		default:fprintf(stderr, "Whaat about token %d ??\n", token->kind);break;
	}
}

void print_tokens(Token *token) {
	while (token) {
		print_token(token);
		token = token->next;
	}
}

bool equal(Token *tok, char *op) {
	return !memcmp(tok->loc, op, tok->len) && strlen(op) == tok->len;
}

Token *skip(Token *tok, char *op) {
	if (!equal(tok, op)) {
		error_tok(tok, "expected '%s'", op);
	}
	return tok->next;
}

bool consume(Token **rest, Token *tok, char *str) {
	if (equal(tok, str)) {
		*rest = tok->next;
		return true;
	}
	*rest = tok;
	return false;
}

// Create a new token and connect it to cur
static Token *new_token(TokenKind kind, char *start, char *end) {
	Token *tok = calloc(1, sizeof(Token));
	tok->kind = kind;
	tok->loc = start;
	tok->len = end - start;
	return tok;
}

static bool startswith(char *p, char *with) {
	return !strncmp(p, with, strlen(with));
}

// Returns true if c is valid as the first character of an identifier.
static bool is_ident1(char c) {
  return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || c == '_';
}

// Returns true if c is valid as a non-first character of an identifier.
static bool is_ident2(char c) {
  return is_ident1(c) || ('0' <= c && c <= '9');
}

static int read_punct(char *p) {
	static char *kw[] = {
		"==", "!=", "<=", ">=", "->", "+=", "-=", "*=", "/=", "++", "--",
		"%=", "&=", "|=", "^=",
	};
	for (int i = 0; i < sizeof(kw)/sizeof(*kw); i++) {
		if (startswith(p, kw[i]))
			return strlen(kw[i]);
	}
	return ispunct(*p) ? 1 : 0;
}

static bool is_keyword(Token *tok) {
  static char *kw[] = {
    "return", "if", "else", "for", "while", "static",
    "void", "_Bool", "char", "short", "int", "long",
    "sizeof", "struct", "union", "enum", "typedef",
  };

  for (int i = 0; i < sizeof(kw) / sizeof(*kw); i++)
    if (equal(tok, kw[i]))
      return true;
  return false;
}

void convert_keywords(Token *tok) {
  for (Token *t = tok; t->kind != TK_EOF; t = t->next)
    if (is_keyword(t))
      t->kind = TK_KEYWORD;
}

static int read_escaped_char(char **new_pos, char *p) {
	int num = *p;
	switch (num) {
		case 'n':num = '\n';p++;break;
		case 'a':num = '\a';p++;break;
		case 'b':num = '\b';p++;break;
		case 't':num = '\t';p++;break;
		case 'v':num = '\v';p++;break;
		case 'f':num = '\f';p++;break;
		case 'r':num = '\r';p++;break;
		case 'e':num = '\e';p++;break;
		case 'x':
			p++;
			num = 0;
			while (isxdigit(*p)) {
//	fprintf(stderr, "seen x=%d (%x) num=%d (%x)\n", *p, *p, num, num);
				num <<= 4;
				if (*p <= '9')
					num += *p -'0';
				else {
					num += (*p | ' ') - 'a' + 10;
				}
				p++;
			}
			break;
		case '0' ... '7': {
			num = 0;
			int count = 0;
			do {
				num <<= 3;
				num += *p++ - '0';
			} while (count++ < 2 && *p >= '0' && *p <= '7');
			break;
		}
		default:
			p++;
			break;
	}
	*new_pos = p;
	return num;
}

static char *string_literal_end(char *p) {
	char *start = p;
	for (; *p != '"'; p++) {
		if (!*p || *p == '\n')
			error_at(p, "non-terminated string");
		if (*p == '\\')
			p++;
	}
	return p;
}

static Token *read_string_literal(char *start) {
//	fprintf(stderr, "FOUND STRING LITTERAL %c\n", *p);
	char *end = string_literal_end(start + 1);
	char *buf = calloc(1, end - start);
	int len = 0;
	for (char *p = start + 1; p < end;) {
		if (*p == '\\')
			buf[len] = read_escaped_char(&p, p + 1);
		else
			buf[len] = *p++;
		len++;
	}
	Token *tok = new_token(TK_STR, start, end + 1);
	tok->ty = array_of(ty_char, len + 1);
	tok->str = buf;
//	fprintf(stderr, "DONE\n");
	return tok;
}

static Token *read_char_literal(char *p) {
//	fprintf(stderr, "FOUND CHARACTER LITTERAL %c\n", *p);
	char *q = p;
	p++;
//	fprintf(stderr, "c=%c\n", *p);
	char c = 0;
	if (*p == '\\') {
		c = read_escaped_char(&p, p + 1);
//		fprintf(stderr, "p=%c\n", *p);
	} else if (*p) {
//		fprintf(stderr, "c=%c\n", *p);
		c = *p++;
	}
	if (*p!='\'') {
		error_at(p, "non-terminated character literal");
	}
	p++;
	Token *tok = new_token(TK_NUM, q, p);
	tok->val = c;
	return tok;
}

static Token *read_int_literal(char *start) {
	char *p = start;

	int base = 10;
	if (!strncasecmp(p, "0x", 2) && isalnum(p[2])) {
		p += 2;
		base = 16;
	} else if (!strncasecmp(p, "0b", 2) && isalnum(p[2])) {
		p += 2;
		base = 2;
	} else if (*p == '0') {
		base = 8;
	}

	long val = strtoul(p, &p, base);
	if (isalnum(*p))
		error_at(p, "invalid digit");

	Token *tok = new_token(TK_NUM, start, p);
	tok->val = val;
	return tok;
}

// tokenize the input string p and return its tokens
static Token *tokenize_string(char *p) {
	user_input = p;
	Token head = {};
	Token *cur = &head;
//	fprintf(stderr, "input string=%s\n", p);
	while (*p) {
		// skip line comments
		if (startswith(p, "//")) {
			p += 2;
			while (*p && *p != '\n') {
				p++;
			}
			continue;
		}
		// skip block comments
		if (startswith(p, "/*")) {
			char *q = strstr(p+2,"*/");
			if (!q) {
				error_at(p, "Non-terminated block comment");
			}
			p = q+2;
			continue;
		}
		// skip whitespace characters
		if (isspace(*p)) {
			p++;
			continue;
		}
		// string literal
		if (*p == '"') {
			cur = cur->next = read_string_literal(p);
			p += cur->len;
			continue;
		}
		// Character literal
		if (*p == '\'') {
			cur = cur->next = read_char_literal(p);
			p += cur->len;
			continue;
		}
		// numeric literal
		if (isdigit(*p)) {
			cur = cur->next = read_int_literal(p);
			p += cur->len;
			continue;
		}
		// identifier or keyword
		if (is_ident1(*p)) {
			char *start = p;
			do {
				p++;
			} while (is_ident2(*p));
			cur = cur->next = new_token(TK_IDENT, start, p);
			continue;
		}
		// punctuators
		int punct_len = read_punct(p);
		if (punct_len) {
			cur = cur->next = new_token(TK_PUNCT, p, p + punct_len);
			p += punct_len;
			continue;
		}
		error_at(p, "invalid token");
	}
	cur = cur->next = new_token(TK_EOF, p, p);
	add_line_numbers(head.next);
	convert_keywords(head.next);
	int do_print_tokens = 0;
	char *env = getenv("PRINT_TOKENS");
	if (env) {
		sscanf(env, "%d", &do_print_tokens);
	}
	if (do_print_tokens) {
		print_tokens(head.next);
	}
	return head.next;
}

static char *read_string(char *input_file) {
	FILE *in;
	if (!strcmp(input_file, "-")) {
		in = stdin;
	} else {
		in = fopen(input_file, "rt");
		if (!in) {
			fprintf(stderr, "Can't open file\n");
			exit(1);
		}
	}
	char *input;
	size_t len;
	FILE *tmp = open_memstream(&input, &len);
	for (;;) {
		char buf[1024];
		if (!fgets(buf, sizeof(buf), in)) {
			break;
		}
		fwrite(buf, strlen(buf), 1, tmp);
	}
	if (in != stdin) {
		fclose(in);
	}
	fflush(tmp);
	return input;
}

// tokenize the input file and return its tokens
Token *tokenize_file(char *input_file) {
	char *input = read_string(input_file);
	user_file = input_file;
	return tokenize_string(input);
}
