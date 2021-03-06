#ifndef __9CC_H__
#define __9CC_H__

#include <stdbool.h>

typedef enum {
	TK_RESERVED,
	TK_RETURN,
	TK_IDENT,
	TK_NUM,
	TK_EOF,
} TokenKind;

typedef struct Token Token;

struct Token {
	TokenKind kind;
	Token *next;
	int val;
	char *str;
	int len;
};

typedef enum {
	ND_RETURN,
	ND_EQU,
	ND_EQUNOT,
	ND_RELINF,
	ND_RELINFEQ,
	ND_RELSUP,
	ND_RELSUPEQ,
	ND_ADD,
	ND_SUB,
	ND_MUL,
	ND_DIV,
	ND_ASSIGN,
	ND_LVAR,
	ND_NUM,
} NodeKind;

typedef struct Node Node;

struct Node {
	NodeKind kind;
	Node *lhs;
	Node *rhs;
	int val;
	int offset;
};

typedef struct LVar LVar;

struct LVar {
	LVar *next;
	char *name;
	int len;
	int offset;
};

extern char *user_input;
extern Token *token;
extern Node *code[100];

void error_at(char *loc, char *fmt, ...);
void error(char *fmt, ...);
bool consume(char *op);
void expect(char *op);
int expect_number();
bool at_eof();
Token *new_token(TokenKind kind, Token *cur, char *str);
Token *tokenize(char *input);
Node *new_node(NodeKind kind, Node *lhs, Node *rhs);
Node *new_node_num(int val);
Node *primary();
Node *unary();
Node *mul();
Node *add();
Node *relational();
Node *equality();
Node *expr();
void parse();
void gen(Node *node);
void generate();

#endif/*__9CC_H__*/
