#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "9cc.h"

Token *token;

#define RED() "\x1b[31m"
#define NRM() "\x1b[0m"
void error_at(char *loc, char *fmt, ...) {
	int pos = loc - user_input;
	va_list ap;
	fprintf(stderr, "<stdin>:1:%d: %serror%s: ", pos + 1, RED(), NRM());
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	fprintf(stderr, "\n");
	fprintf(stderr, "%s\n", user_input);
	fprintf(stderr, "%*s", pos, " ");
	fprintf(stderr, "^\n");
	exit(1);
}

// Function to report an error
// takes the same arguments as printf
void error(char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	fprintf(stderr, "\n");
	exit(1);
}

int is_alnum(char c) {
	return ('a' <= c && c <= 'z') ||
		('A' <= c && c <= 'Z') ||
		('0' <= c && c <= '9') ||
		(c == '_');
}

void print(char *s) {
	fprintf(stderr, "%s\n", s);
}

void print_str(char *s, Token *token) {
	char buf[1024];
	snprintf(buf, token->len + 1, "%s", token->str);
	fprintf(stderr, "%s '%s'\n", s, buf);
}

void print_num(char *s, Token *token) {
	fprintf(stderr, "%s '%d'\n", s, token->val);
}

void print_token(Token *token) {
	if (!token) {
		return;
	}
	switch (token->kind) {
		case TK_RETURN:print("RETURN");break;
		case TK_IF:print("IF");break;
		case TK_ELSE:print("ELSE");break;
		case TK_WHILE:print("WHILE");break;
		case TK_FOR:print("FOR");break;
		case TK_IDENT:
			print_str("IDENT", token);
			break;
		case TK_NUM:
			print_num("NUM", token);
			break;
		case TK_RESERVED:
			print_str("RESVD", token);
			break;
		case TK_EOF:fprintf(stderr, "EOF\n");break;
		default:fprintf(stderr, "Whaat about token %d ??\n", token->kind);break;
	}
}

void print_tokens(Token *token) {
	int cont = 1;
	while (token) {
		print_token(token);
		if (token->kind == TK_EOF) {
			break;
		}
		token = token->next;
	}
}

// When the next token is the expected symbol, read the token one more time and
// Return true. Otherwise, return false.
bool consume(char *op) {
	// 👻 Important : we must both check token len AND token contents ! else "=" can be taken for "=="
	if (token->kind != TK_RESERVED || strlen(op) != token->len || memcmp(token->str, op, token->len)) {
		return false;
	}
	token = token->next;
	return true;
}

bool consume_keyword(char *op) {
	// if (strncmp(token->str, op, token->len)) {
	if (strlen(op) != token->len || memcmp(token->str, op, token->len)) {
		return false;
	}
	token = token->next;
	return true;
}

Token *consume_ident() {
	Token *tok = 0;
	if (token->kind == TK_IDENT) {
		tok = token;
		token = token->next;
	}
	return tok;
}

// If the next token is the expected symbol, read the token one more time.
// Otherwise, report an error.
void expect(char *op) {
	if (token->kind != TK_RESERVED || strncmp(token->str, op, token->len)) {
		error_at(token->str, "expected '%c'", *op);
	}
	token = token->next;
}

// If the next token is a number, read the token one more time and return the number.
// Otherwise, report an error.
int expect_number() {
	if (token->kind != TK_NUM) {
		error_at(token->str, "It's not a number");
	}
	int val = token->val;
	token = token->next;
	return val;
}

bool at_eof() {
	return token->kind == TK_EOF;
}

// Create a new token and connect it to cur
Token *new_token(TokenKind kind, Token *cur, char *str) {
	Token *tok = calloc(1, sizeof(Token));
	tok->kind = kind;
	tok->str = str;
	cur->next = tok;
	// print_token(tok);
	return tok;
}

char *user_input;

bool is_keyword(char *p, char *keyword) {
	int len = strlen(keyword);
	return !strncmp(p, keyword, len) && !is_alnum(p[len]);
}
// tokenize the input string p and return it
Token *tokenize(char *input) {
	user_input = input;
	char *p = user_input;
	Token head;
	head.next = NULL;
	Token *cur = &head;
	while (*p) {
		if (isspace(*p)) {
			p++;
			continue;
		}
		if (!strncmp(p, "//", 2)) {
			p += 2;
			while (*p && *p != '\n') {
				p++;
			}
			continue;
		}
		// Multi-char reserver keywords
		if (strchr("<>=!", *p)) {
			int len = 0;
			if (!strncmp(p, "<=", 2)) {
				len = 2;
			} else if (!strncmp(p, ">=", 2)) {
				len = 2;
			} else if (!strncmp(p, "==", 2)) {
				len = 2;
			} else if (!strncmp(p, "!=", 2)) {
				len = 2;
			}
			if (len) {
				cur = new_token(TK_RESERVED, cur, p);
				p += len;
				cur->len = len;
				continue;
			}
		}
		// One char reserved keywords
		if (strchr("+-*/()<>;={}", *p)) {
			cur = new_token(TK_RESERVED, cur, p++);
			cur->len = 1;
			continue;
		} else if (isdigit(*p)) {
			cur = new_token(TK_NUM, cur, p);
			char *old_p = p;
			cur->val = strtol(p, &p, 10);
			cur->len = p - old_p;
			continue;
		} else if (is_keyword(p, "return")) {
			cur = new_token(TK_RETURN, cur, p);
			cur->len = 6;
			p += cur->len;
			continue;
		} else if (is_keyword(p, "if")) {
			cur = new_token(TK_IF, cur, p);
			cur->len = 2;
			p += cur->len;
			continue;
		} else if (is_keyword(p, "else")) {
			cur = new_token(TK_ELSE, cur, p);
			cur->len = 4;
			p += cur->len;
			continue;
		} else if (is_keyword(p, "while")) {
			cur = new_token(TK_WHILE, cur, p);
			cur->len = 5;
			p += cur->len;
			continue;
		} else if (is_keyword(p, "for")) {
			cur = new_token(TK_FOR, cur, p);
			cur->len = 3;
			p += cur->len;
			continue;
		} else if (is_alnum(*p)) {
			char *old_p = p;
			while (is_alnum(*p)) {
				p++;
			}
			cur = new_token(TK_IDENT, cur, old_p);
			cur->len = p - old_p;
			continue;
		}
		error_at(p, "I can't tokenize '%c'", *p);
	}
	new_token(TK_EOF, cur, p);
	token = head.next;
	int do_print_tokens = 0;
	char *env = getenv("PRINT_TOKENS");
	if (env) {
		sscanf(env, "%d", &do_print_tokens);
	}
	if (do_print_tokens) {
		print_tokens(token);
	}
	return head.next;
}
