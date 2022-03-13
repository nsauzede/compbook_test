#ifndef __9CC_H__
#define __9CC_H__

#include <stdbool.h>

typedef struct Type Type;
typedef struct Node Node;
typedef struct Member Member;

typedef enum {
	TK_IDENT,
	TK_PUNCT,
	TK_KEYWORD,
	TK_STR,
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
	Type *ty;	// used if TK_STR
	char *str;	// string litteral, including terminating 0

	int line_no;
};

void error(char *fmt, ...);
void error_at(char *loc, char *fmt, ...);
#define error_tok(t, f...) do { \
		fprintf(stderr, "%s:%d:%s:", __FILE__, __LINE__, __FUNCTION__); \
		_error_tok(t, f); \
	} while (0);
void _error_tok(Token *tok, char *fmt, ...);
bool equal(Token *tok, char *op);
Token *skip(Token *tok, char *op);
bool consume(Token **rest, Token *tok, char *str);
void convert_keywords(Token *tok);
Token *tokenize_file(char *input_file);

Token *preprocess(Token *tok);

typedef struct Obj Obj;
struct Obj {
	Obj *next;
	char *name;
	Type *ty;
	bool is_local;
	// local variable
	int offset;
	// global variable or function
	bool is_function;
	// global variable
	char *init_data;
	// function
	Obj *params;
	Node *body;
	Obj *locals;
	int stack_size;
};

typedef enum {
	ND_ADD,			// +
	ND_SUB,			// -
	ND_MUL,			// *
	ND_DIV,			// /
	ND_NEG,			// unary -
	ND_EQ,			// ==
	ND_NE,			// !=
	ND_LT,			// <
	ND_LE,			// <=
	ND_ASSIGN,		// =
	ND_COMMA,		// ,
	ND_MEMBER,		// . (struct member access)
	ND_ADDR,		// unary &
	ND_DEREF,		// unary *
	ND_RETURN,		//
	ND_IF,			//
	ND_FOR,			//
	ND_BLOCK,		//
	ND_FUNCALL,		//
	ND_EXPR_STMT,		//
	ND_STMT_EXPR,		//
	ND_VAR,			//
	ND_NUM,			// integer/long/char
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
	// for block or statement expression
	Node *body;
	// for IF/WHILE/FOR :
	Node *init;	// FOR
	Node *cond;
	Node *inc;	// FOR
	Node *then;
	Node *els;

	// Struct member access
	Member *member;

	// function call	
	char *funcname;
	Node *args;
	
	Obj *var;	// VAR
	long val;	// NUM
};

Obj *parse(Token *tok);

typedef enum {
	TY_INT,
	TY_LONG,
	TY_CHAR,
	TY_PTR,
	TY_FUNC,
	TY_ARRAY,
	TY_STRUCT,
} TypeKind;

struct Type {
	TypeKind kind;
	int size;		// sizeof() value
	Type *base;
	Token *name;
	int array_len;

	// Struct
	Member *members;

	// function
	Type *return_ty;
	Type *params;
	Type *next;
};

// Struct member
struct Member {
	Member *next;
	Type *ty;
	Token *name;
	int offset;
};

extern Type *ty_int;
extern Type *ty_long;
extern Type *ty_char;

bool is_integer(Type *ty);
Type *pointer_to(Type *base);
Type *copy_type(Type *ty);
Type *func_type(Type *return_ty);
Type *array_of(Type *base, int size);
void add_type(Node *node);

char *codegen(Obj *prog, char *input_file);

#endif/*__9CC_H__*/
