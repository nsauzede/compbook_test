#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "9cc.h"

void error_at(char *loc, char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	int pos = loc - user_input;
	fprintf(stderr, "%s\n", user_input);
	fprintf(stderr, "%*s", pos, " ");
	fprintf(stderr, "^ ");
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	fprintf(stderr, "\n");
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

// When the next token is the expected symbol, read the token one more time and
// Return true. Otherwise, return false.
bool consume(char *op) {
	if (token->kind != TK_RESERVED || strncmp(token->str, op, token->len)) {
		return false;
	}
	token = token->next;
	return true;
}

// If the next token is the expected symbol, read the token one more time.
// Otherwise, report an error.
void expect(char *op) {
	if (token->kind != TK_RESERVED || strncmp(token->str, op, token->len)) {
		error_at(token->str, "It's not '%c'", op);
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
	return tok;
}

// tokenize the input string p and return it
Token *tokenize(char *p) {
	Token head;
	head.next = NULL;
	Token *cur = &head;
	while (*p) {
		if (isspace(*p)) {
			p++;
			continue;
		} else if (strchr("<>=!", *p)) {
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
				cur->len = len;
				p += len;
				continue;
			}
		}
		if (strchr("+-*/()<>", *p)) {
			cur = new_token(TK_RESERVED, cur, p);
			cur->len = 1;
			p++;
			continue;
		} else if (isdigit(*p)) {
			cur = new_token(TK_NUM, cur, p);
			cur->val = strtol(p, &p, 10);
			continue;
		}
		error_at(p, "I can't tokenize '%c'", *p);
	}
	new_token(TK_EOF, cur, p);
	return head.next;
}

Node *new_node(NodeKind kind, Node *lhs, Node *rhs) {
	Node *node = calloc(1, sizeof(Node));
	node->kind = kind;
	node->lhs = lhs;
	node->rhs = rhs;
	return node;
}

Node *new_node_num(int val) {
	Node *node = calloc(1, sizeof(Node));
	node->kind = ND_NUM;
	node->val = val;
	return node;
}

Node *expr();

Node *primary() {
	if (consume("(")) {
		Node *node = expr();
		expect(")");
		return node;
	}
	return new_node_num(expect_number());
}

Node *unary() {
	if (consume("+")) {
		return primary();
	} else if (consume("-")) {
		return new_node(ND_SUB, new_node_num(0), primary());
	}
	return primary();
}

Node *mul() {
	Node *node = unary();
	for (;;) {
		if (consume("*")) {
			node = new_node(ND_MUL, node, unary());
		} else if (consume("/")) {
			node = new_node(ND_DIV, node, unary());
		} else {
			return node;
		}
	}
}

Node *add() {
	Node *node = mul();
	for (;;) {
		if (consume("+")) {
			node = new_node(ND_ADD, node, mul());
		} else if (consume("-")) {
			node = new_node(ND_SUB, node, mul());
		} else {
			return node;
		}
	}
}

Node *relational() {
	Node *node = add();
	for (;;) {
		if (consume("<")) {
			node = new_node(ND_RELINF, node, add());
		} else if (consume("<=")) {
			node = new_node(ND_RELINFEQ, node, add());
		} else if (consume(">")) {
			node = new_node(ND_RELSUP, node, add());
		} else if (consume(">=")) {
			node = new_node(ND_RELSUPEQ, node, add());
		} else {
			return node;
		}
	}
}

Node *equality() {
	Node *node = relational();
	for (;;) {
		if (consume("==")) {
			node = new_node(ND_EQU, node, relational());
		} else if (consume("!=")) {
			node = new_node(ND_EQUNOT, node, relational());
		} else {
			return node;
		}
	}
}

Node *expr() {
	Node *node = equality();
	return node;
}
