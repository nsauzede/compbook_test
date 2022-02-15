#ifndef __9CC_H__
#define __9CC_H__

#include <stdbool.h>

typedef struct Node Node;

typedef enum {
	TK_IDENT,
	TK_PUNCT,
	TK_KEYWORD,
	TK_NUM,
	TK_EOF,
} TokenKind;

typedef struct Token Token;
struct Token {
	TokenKind kind;
	Token *next;
	long val;
	char *loc;
	int len;
};

void error(char *fmt, ...);
void error_at(char *loc, char *fmt, ...);
void error_tok(Token *tok, char *fmt, ...);
bool equal(Token *tok, char *op);
Token *skip(Token *tok, char *op);
bool consume(Token **rest, Token *tok, char *str);
Token *tokenize(char *input);

typedef struct Type Type;
typedef struct Obj Obj;
struct Obj {
	Obj *next;
	char *name;
	Type *ty;
	bool is_function;
	bool is_local;
	// local variable
	int offset;
	// function
	Obj *params;
	Node *body;
	Obj *locals;
	int stack_size;
	// for V
	bool already_assigned;
	bool is_param;
};

typedef enum {
	ND_ADD,			// 0	+
	ND_SUB,			// 1	-
	ND_MUL,			// 2	*
	ND_DIV,			// 3	/
	ND_NEG,			// 4	unary -
	ND_EQ,			// 5	==
	ND_NE,			// 6	!=
	ND_LT,			// 7	<
	ND_LE,			// 8	<=
	ND_ASSIGN,		// 9	=
	ND_ADDR,		// 10	unary &
	ND_DEREF,		// 11	unary *
	ND_RETURN,		// 12
	ND_IF,			// 13
	ND_FOR,			// 14
	ND_BLOCK,		// 15
	ND_FUNCALL,		// 16
	ND_EXPR_STMT,	// 17
	ND_VAR,			// 18
	ND_NUM,			// 19	integer/long
} NodeKind;

typedef struct LVar LVar;
struct LVar {
	LVar *next;
	char *name;
	int len;
	int offset;	// var offset in stack, in word machine multiple (eg: 8 on x64)
};

struct Node {
	NodeKind kind;
	Token *tok;	// related token
	Type *ty;	// Type, eg: int  or pointer to int
	Node *next;

	Node *lhs;
	Node *rhs;

	Node *body;	// for BLOCK
	// for IF/WHILE/FOR :
	Node *init;	// FOR
	Node *cond;
	Node *inc;	// FOR
	Node *then;
	Node *els;
	// function call	
	char *funcname;
	Node *args;
	
	Obj *var;	// VAR
	long val;	// NUM

// For V codegen :
	bool paren;		// false=>none, true=>this node is within '()'
	bool boolean;	// expr is a boolean
	bool pointer_arith;
};

Obj *parse(Token *tok);

typedef enum {
	TY_INT,
	TY_PTR,
	TY_FUNC,
	TY_ARRAY,
} TypeKind;

struct Type {
	TypeKind kind;
	int size;		// sizeof() value
	Type *base;
	Token *name;
	int array_len;
	// function
	Type *return_ty;
	Type *params;
	Type *next;
};

extern Type *ty_int;

bool is_integer(Type *ty);
Type *pointer_to(Type *base);
Type *copy_type(Type *ty);
Type *func_type(Type *return_ty);
Type *array_of(Type *base, int size);
void add_type(Node *node);

void codegen(Obj *prog, char *input);

#endif/*__9CC_H__*/
