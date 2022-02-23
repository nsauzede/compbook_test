#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "chibicc.h"

Token *token;

#define RED() "\x1b[31m"
#define NRM() "\x1b[0m"
static char *user_input;
static char *user_file = "<stdin>";
static char *get_line_col(int *line, int *col, int pos) {
	char *str = user_input;
	*line = 1;
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
	str = strdup(str);
	char *p = strchr(str, '\n');
	if (p) {
		*p = 0;
	}
	return str;
}
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
		fprintf(stderr, "%s\n", str);
		fprintf(stderr, "%*s", pos, "");
		fprintf(stderr, "^ len=%d\n", len);
	}
	exit(1);
}

void error(char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	verror_at(0, 0, fmt, ap);
}

void error_at(char *loc, char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	verror_at(loc, 0, fmt, ap);
}

void _error_tok(Token *tok, char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	verror_at(tok->loc, tok->len, fmt, ap);
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
	if (startswith(p, "==") || startswith(p, "!=") ||
		startswith(p, "<=") || startswith(p, ">=")) {
		return 2;
	}
	return ispunct(*p) ? 1 : 0;
}

static bool is_keyword(Token *tok) {
  static char *kw[] = {
    "return", "if", "else", "for", "while", "int", "long", "char", "sizeof",
  };

  for (int i = 0; i < sizeof(kw) / sizeof(*kw); i++)
    if (equal(tok, kw[i]))
      return true;
  return false;
}

static void convert_keywords(Token *tok) {
  for (Token *t = tok; t->kind != TK_EOF; t = t->next)
    if (is_keyword(t))
      t->kind = TK_KEYWORD;
}

// tokenize the input string p and return its tokens
static Token *tokenize_string(char *p) {
	user_input = p;
	Token head = {};
	Token *cur = &head;
//	fprintf(stderr, "input string=%s\n", p);
	while (*p) {
		if (isspace(*p)) {
			p++;
			continue;
		}
		if (startswith(p, "//")) {
			p += 2;
			while (*p && *p != '\n') {
				p++;
			}
			continue;
		}
		if (startswith(p, "/*")) {
			char *q = strstr(p+2,"*/");
			if (!q) {
				error_at(p, "Non-terminated block comment");
			}
			p = q+2;
			continue;
		}
		if (startswith(p, "\"")) {
//			fprintf(stderr, "FOUND STRING LITTERAL %c\n", *p);
			char *q = p;
			p++;
			int esc = 0;
			while (*p) {
//				fprintf(stderr, "c=%c\n", *p);
				if (!esc) {
					if (*p == '"') {
						break;
					}
					if (*p == '\\') {
						esc = 1;
					}
				} else {
					esc = 0;
				}
				p++;
			}
			if (!*p || *p!='"') {
				error_at(q, "non-terminated string");
			}
			p++;
			cur = cur->next = new_token(TK_STR, q, p);
			cur->str = strndup(q + 1, p - q - 2);
			cur->ty = array_of(ty_char, p - q - 1);
//			fprintf(stderr, "DONE\n");
			continue;
		}
		if (isdigit(*p)) {
			cur = cur->next = new_token(TK_NUM, p, p);
			char *q = p;
			cur->val = strtoul(p, &p, 10);
			cur->len = p - q;
			continue;
		}
		if (is_ident1(*p)) {
			char *start = p;
			do {
				p++;
			} while (is_ident2(*p));
			cur = cur->next = new_token(TK_IDENT, start, p);
			continue;
		}
		int punct_len = read_punct(p);
		if (punct_len) {
			cur = cur->next = new_token(TK_PUNCT, p, p + punct_len);
			p += punct_len;
			continue;
		}
		error_at(p, "invalid token");
	}
	cur = cur->next = new_token(TK_EOF, p, p);
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
Token *tokenize(char *input_file) {
	char *input = read_string(input_file);
	user_file = input_file;
	return tokenize_string(input);
}
