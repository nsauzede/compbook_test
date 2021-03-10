#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
	TK_RESERVED,
	TK_NUM,
	TK_EOF,
} TokenKind;

typedef struct Token Token;
struct Token {
	TokenKind kind;
	Token *next;
	int val;
	char *str;
};

Token *token;
char *NodeNames[] = {
	"ADD",
	"SUB",
	"MUL",
	"DIV",
	"NUM",
};
typedef enum {
	ND_ADD,
	ND_SUB,
	ND_MUL,
	ND_DIV,
	ND_NUM,
} NodeKind;
typedef struct Node Node;
struct Node {
	NodeKind kind;
	Node *lhs;
	Node *rhs;
	int val;
	bool paren;	// false=>none, true=>this node is within '()'
};

char *user_input;

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

void info(char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	fprintf(stderr, "\n");
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
bool consume(char op) {
	if (token->kind != TK_RESERVED || token->str[0] != op) {
		return false;
	}
	token = token->next;
	return true;
}

// If the next token is the expected symbol, read the token one more time.
// Otherwise, report an error.
void expect(char op) {
	if (token->kind != TK_RESERVED || token->str[0] != op) {
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
		}
		if (strchr("+-*/()", *p)) {
			cur = new_token(TK_RESERVED, cur, p++);
			continue;
		}
		if (isdigit(*p)) {
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
	if (consume('(')) {
		Node *node = expr();
		expect(')');
		node->paren = true;
		return node;
	}
	return new_node_num(expect_number());
}
Node *mul() {
	Node *node = primary();
	for (;;) {
		if (consume('*')) {
			node = new_node(ND_MUL, node, primary());
		} else if (consume('/')) {
			node = new_node(ND_DIV, node, primary());
		} else {
			return node;
		}
	}
}

Node *expr() {
	Node *node = mul();
	for (;;) {
		if (consume('+')) {
			node = new_node(ND_ADD, node, mul());
		} else if (consume('-')) {
			node = new_node(ND_SUB, node, mul());
		} else {
			return node;
		}
	}
}

void gen(Node *node) {
	// info("%s", NodeNames[node->kind]);
	if (node->paren) {
		printf("(");
	}
	if (node->kind == ND_NUM) {
		printf("%d", node->val);
		return;
	}
	gen(node->lhs);
	switch (node->kind) {
		case ND_ADD:
			printf("+");
			break;
		case ND_SUB:
			printf("-");
			break;
		case ND_MUL:
			printf("*");
			break;
		case ND_DIV:
			printf("/");
			break;
	}
	gen(node->rhs);
	if (node->paren) {
		printf(")");
	}
}

int main(int argc, char *argv[]) {
	if (argc != 2) {
		fprintf(stderr, "Usage: %s <number>\n", argv[0]);
		return 1;
	}
	user_input = argv[1];
	token = tokenize(user_input);
	Node *node = expr();
	printf("exit(");
	gen(node);
	printf(")\n");
	return 0;
}
