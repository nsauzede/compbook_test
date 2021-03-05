#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "9cc.h"

Token *token;
LVar g_empty_local;
LVar *locals = &g_empty_local;

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
void print_token(Token *token) {
	if (!token) {
		return;
	}
	switch (token->kind) {
		case TK_IDENT:fprintf(stderr, "IDENT\n");break;
		case TK_NUM:fprintf(stderr, "NUM\n");break;
		case TK_RESERVED:fprintf(stderr, "RESVD\n");break;
		case TK_EOF:fprintf(stderr, "EOF\n");break;
		default:fprintf(stderr, "Whaat about ??\n");break;
	}
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

void print_str(char *s, Token *token) {
	char buf[1024];
	snprintf(buf, token->len + 1, "%s", token->str);
	fprintf(stderr, "%s '%s'\n", s, buf);
}

void print_num(char *s, Token *token) {
	fprintf(stderr, "%s '%d'\n", s, token->val);
}

void print_tokens(Token *token) {
	int cont = 1;
	while (cont) {
		if (!token) {
			break;
		}
		switch (token->kind) {
			case TK_IDENT:
				print_str("IDENT", token);
				break;
			case TK_NUM:
				print_num("NUM", token);
				break;
			case TK_RESERVED:
				print_str("RESVD", token);
				break;
			case TK_EOF:fprintf(stderr, "EOF\n");cont=0;break;
			default:fprintf(stderr, "Whaat about token %d ??\n", token->kind);break;
		}
		token = token->next;
	}
}

void print_node(Node *node) {
	if (!node) {
		return;
	}
	if (node->lhs) {
		print_node(node->lhs);
	}
	switch (node->kind) {
		case ND_EQU:fprintf(stderr, "EQU\n");break;
		case ND_LVAR:fprintf(stderr, "LVAR\n");break;
		case ND_NUM:fprintf(stderr, "NUM\n");break;
		case ND_ASSIGN:fprintf(stderr, "ASSIGN\n");break;
		case ND_ADD:fprintf(stderr, "ADD\n");break;
		default:fprintf(stderr, "Whaat about node %d ??\n", node->kind);break;
	}
	if (node->rhs) {
		print_node(node->rhs);
	}
}

void print_nodes(Node **nodes) {
	if (!nodes) {
		return;
	}
	while (1) {
		if (!*nodes) {
			break;
		}
		fprintf(stderr, "Statement :\n");
		print_node(*nodes);
		nodes++;
	}
}

// tokenize the input string p and return it
Token *tokenize() {
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
		if (strchr("+-*/()<>;=", *p)) {
			cur = new_token(TK_RESERVED, cur, p++);
			cur->len = 1;
			continue;
		} else if (isdigit(*p)) {
			cur = new_token(TK_NUM, cur, p);
			char *old_p = p;
			cur->val = strtol(p, &p, 10);
			cur->len = p - old_p;
			continue;
		} else if ('a' <= *p && *p <= 'z') {
			char *old_p = p;
			while ('a' <= *p && *p <= 'z') {
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

LVar *find_lvar(Token *tok) {
	for (LVar *var = locals; var; var = var->next) {
		if (var->len == tok->len && !memcmp(tok->str, var->name, var->len)) {
			return var;
		}
	}
	return NULL;
}

Node *primary() {
	if (consume("(")) {
		Node *node = expr();
		expect(")");
		return node;
	}
	Token *tok = consume_ident();
	if (tok) {
		Node *node = calloc(1, sizeof(Node));
		node->kind = ND_LVAR;
		LVar *lvar = find_lvar(tok);
		if (lvar) {
			node->offset = lvar->offset;
		} else {
			lvar = calloc(1, sizeof(LVar));
			lvar->next = locals;
			lvar->name = tok->str;
			lvar->len = tok->len;
			lvar->offset = locals->offset + 8;
			node->offset = lvar->offset;
			locals = lvar;
		}
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
			// fprintf(stderr, "OUTPUT EQU !!\n");
			node = new_node(ND_EQU, node, relational());
		} else if (consume("!=")) {
			node = new_node(ND_EQUNOT, node, relational());
		} else {
			return node;
		}
	}
}

Node *assign() {
	Node *node = equality();
	if (consume("=")) {
		// fprintf(stderr, "OUTPUT ASSIGN !!\n");
		node = new_node(ND_ASSIGN, node, assign());
	}
	return node;
}

Node *expr() {
	return assign();
}

Node *stmt() {
	Node *node = expr();
	expect(";");
	return node;
}

void program() {
	int i = 0;
	while (!at_eof()) {
		code[i++] = stmt();
	}
	code[i] = NULL;
	int do_print_nodes = 0;
	char *env = getenv("PRINT_NODES");
	if (env) {
		sscanf(env, "%d", &do_print_nodes);
	}
	if (do_print_nodes) {
		print_nodes(code);
	}
}