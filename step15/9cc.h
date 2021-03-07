#ifndef __9CC_H__
#define __9CC_H__

#include <stdbool.h>

typedef enum {
	TK_RESERVED,
	TK_RETURN,
	TK_IF,
	TK_ELSE,
	TK_WHILE,
	TK_FOR,
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
	ND_BLOCK,
	ND_CALL,
	ND_RETURN,
	ND_IF,
	ND_ELSE,
	ND_WHILE,
	ND_FOR,
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
	Node *lhs;	// for FUNC : lvar of the func ident
	Node *rhs;	// for CALL ARGS : arg list via subsequent ->next
				// for FUNC : list of args via lvar->next
	Node *next;	// for BLOCK : compound stmt list via subsequent ->next
				// for PROGRAM : list of FUNC
	int val;
	int offset;	// for IF/ELSE/WHILE : label number
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
Token *consume_ident();
bool consume_keyword(char *op);
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
Node *stmt();
void parse();
void gen(Node *node);
void generate();

#endif/*__9CC_H__*/
