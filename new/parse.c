#define _POSIX_C_SOURCE 200809L
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "chibicc.h"

static int do_print_ast = 0;
static Obj *globals;
static Obj *locals;

static Node *new_add(Node *lhs, Node *rhs, Token *tok);
static Node *stmt(Token **rest, Token *tok);
static Node *expr(Token **rest, Token *tok);
static Type *declarator(Token **rest, Token *tok, Type *ty);
static Node *declaration(Token **rest, Token *tok);
static Node *assign(Token **rest, Token *tok);
static Node *postfix(Token **rest, Token *tok);
static Node *unary(Token **rest, Token *tok);
static Node *funcall(Token **rest, Token *tok);

static void print_node(Node *node, int depth) {
	if (!node) {
		return;
	}
	if (!node->lhs) {
		// fprintf(stderr, "%*s%d", depth, "", depth);
		fprintf(stderr, "%*s", depth, "");
	}
	if (node->kind == ND_FUNCALL) {
		LVar *lvar = (LVar *)node->lhs;
		char buf[1000];
		snprintf(buf, lvar->len + 1, "%s", lvar->name);
		fprintf(stderr, "CALL %s()\n", buf);
		return;
	}
	print_node(node->lhs, depth);
	switch (node->kind) {
		case ND_BLOCK: {
			fprintf(stderr, "BLOCK %p,%p\n", node->lhs, node->rhs);
			for (Node *n = node->body; n; n = n->next) {
				print_node(n, depth + 1);
			}
			break;
		}
		case ND_RETURN:fprintf(stderr, "RETURN\n");break;
		case ND_SUB:fprintf(stderr, "SUB\n");break;
		case ND_MUL:fprintf(stderr, "MUL\n");break;
		case ND_DIV:fprintf(stderr, "DIV\n");break;
		case ND_IF:fprintf(stderr, "IF\n");break;
		case ND_FOR:fprintf(stderr, "FOR\n");break;
		case ND_EQ:fprintf(stderr, "EQ\n");break;
		case ND_VAR:fprintf(stderr, "LVAR\n");break;
		case ND_NUM:fprintf(stderr, "NUM %d\n", node->val);break;
		case ND_ASSIGN:fprintf(stderr, "ASSIGN\n");break;
		case ND_ADDR:fprintf(stderr, "ADDR\n");break;
		case ND_DEREF:fprintf(stderr, "DEREF\n");break;
		case ND_ADD:fprintf(stderr, "ADD\n");break;
		case ND_LT:fprintf(stderr, "INF\n");break;
		case ND_EXPR_STMT:fprintf(stderr, "EXPR_STMT\n");break;
		default:fprintf(stderr, "Whaat about node %d ??\n", node->kind);break;
	}
	print_node(node->rhs, depth);
}

static void print_obj(Obj *obj, int depth) {
	if (!obj)return;
	fprintf(stderr, "Obj %s : %s %s\n", obj->name, obj->is_local ? "local" : "global", obj->is_function ? "function" : "var");
	if (obj->is_function) {
		print_node(obj->body, depth + 1);
	}
	print_obj(obj->next, depth);
}

static void print_nodes(Node **nodes) {
	if (!nodes) {
		return;
	}
	while (1) {
		if (!*nodes) {
			break;
		}
		fprintf(stderr, "Statement :\n");
		print_node(*nodes, 0);
		nodes++;
	}
}

static Obj *find_var(Token *tok) {
	for (Obj *var = locals; var; var = var->next) {
		if (strlen(var->name) == tok->len && !strncmp(tok->loc, var->name, tok->len)) {
			return var;
		}
	}
	for (Obj *var = globals; var; var = var->next) {
		if (strlen(var->name) == tok->len && !strncmp(tok->loc, var->name, tok->len)) {
			return var;
		}
	}
	return NULL;
}

static Node *new_node(NodeKind kind, Token *tok) {
	Node *node = calloc(1, sizeof(Node));
	node->kind = kind;
	node->tok = tok;
	return node;
}

static Node *new_binary(NodeKind kind, Node *lhs, Node *rhs, Token *tok) {
	Node *node = new_node(kind, tok);
	node->lhs = lhs;
	node->rhs = rhs;
	// if (do_print_ast)
	// 	print_node(node, 0);
	return node;
}

static Node *new_unary(NodeKind kind, Node *expr, Token *tok) {
	Node *node = new_node(kind, tok);
	node->lhs = expr;
	// if (do_print_ast)
	// 	print_node(node, 0);
	return node;
}

static Node *new_num(int val, Token *tok) {
	Node *node = new_node(ND_NUM, tok);
	node->val = val;
	// if (do_print_ast)
	// 	print_node(node, 0);
	return node;
}

static Node *new_var_node(Obj *var, Token *tok) {
  Node *node = new_node(ND_VAR, tok);
  node->var = var;
  return node;
}

static Obj *new_var(char *name, Type *ty) {
  Obj *var = calloc(1, sizeof(Obj));
  var->name = name;
  var->ty = ty;
  return var;
}

static Obj *new_lvar(char *name, Type *ty) {
  Obj *var = new_var(name, ty);
  var->is_local = true;
  var->next = locals;
  locals = var;
  return var;
}

static Obj *new_gvar(char *name, Type *ty) {
  Obj *var = new_var(name, ty);
  var->next = globals;
  globals = var;
  return var;
}

static char *get_ident(Token *tok) {
	if (tok->kind != TK_IDENT) {
		error_tok(tok, "expected an identifier");
	}
	return strndup(tok->loc, tok->len);
}

static int get_number(Token *tok) {
  if (tok->kind != TK_NUM)
    error_tok(tok, "expected a number");
  return tok->val;
}

static Type *declspec(Token **rest, Token *tok) {
	if (!consume(rest, tok, "long")) {
		*rest = skip(tok, "int");
	}
	return ty_int;
}

static Type *func_params(Token **rest, Token *tok, Type *ty) {
	Type head = {};
	Type *cur = &head;
	while (!equal(tok, ")")) {
		if (cur != &head) {
			tok = skip(tok, ",");
		}
		Type *basety = declspec(&tok, tok);
		Type *ty = declarator(&tok, tok, basety);
		cur = cur->next = copy_type(ty);
	}
	ty = func_type(ty);
	ty->params = head.next;
	*rest = tok->next;
	return ty;
}

static Type *type_suffix(Token **rest, Token *tok, Type *ty) {
	if (equal(tok, "(")) {
		return func_params(rest, tok->next, ty);
	}
	if (equal(tok, "[")) {
		int sz = get_number(tok->next);
		tok = skip(tok->next->next, "]");
		ty = type_suffix(rest, tok, ty);
		return array_of(ty, sz);
	}
	*rest = tok;
	return ty;
}

static Type *declarator(Token **rest, Token *tok, Type *ty) {
	while (consume(&tok, tok, "*")) {
		ty = pointer_to(ty);
	}

	if (tok->kind != TK_IDENT) {
		error_tok(tok, "expected a variable name");
	}
	ty = type_suffix(rest, tok->next, ty);
	ty->name = tok;
	return ty;
}

static Node *primary(Token **rest, Token *tok) {
	if (equal(tok, "(")) {
		Node *node = expr(&tok, tok->next);
		*rest = skip(tok, ")");
		node->paren = true;
		return node;
	} else if (equal(tok, "sizeof")) {
		Node *node = unary(rest, tok->next);
		add_type(node);
		return new_num(node->ty->size, tok);
	} else if (tok->kind == TK_IDENT) {
		if (equal(tok->next, "(")) {
			return funcall(rest, tok);
		}
		Obj *var = find_var(tok);
		if (!var) {
			char buf[100];
			int len = sizeof(buf);
			if (len > tok->len + 1) len = tok->len + 1;
			snprintf(buf, len, "%s", tok->loc);
			error_tok(tok, "undefined variable '%s'", buf);
		}
		*rest = tok->next;
		return new_var_node(var, tok);
	} else if (tok->kind == TK_NUM) {
		Node *node = new_num(tok->val, tok);
		*rest = tok->next;
		return node;
	}
	error_tok(tok, "expected an expression");
}

static Node *funcall(Token **rest, Token *tok) {
	Token *start = tok;
	tok = tok->next->next;
	Node head = {};
	Node *cur = &head;
	while (!equal(tok, ")")) {
		if (cur != &head) {
			tok = skip(tok, ",");
		}
		cur = cur->next = assign(&tok, tok);
	}
	*rest = skip(tok, ")");
	Node *node = new_node(ND_FUNCALL, start);
	node->funcname = strndup(start->loc, start->len);
	node->args = head.next;
	return node;
}
static Node *unary(Token **rest, Token *tok) {
	if (equal(tok, "+")) {
		return unary(rest, tok->next);
	} else if (equal(tok, "-")) {
		return new_unary(ND_NEG, unary(rest, tok->next), tok);
	} else if (equal(tok, "&")) {
		return new_unary(ND_ADDR, unary(rest, tok->next), tok);
	} else if (equal(tok, "*")) {
		return new_unary(ND_DEREF, unary(rest, tok->next), tok);
	}
	return postfix(rest, tok);
}

static Node *postfix(Token **rest, Token *tok) {
	Node *node = primary(&tok, tok);
	while (equal(tok, "[")) {
		// x[y] is short for *(x+y)
		Token *start = tok;
		Node *idx = expr(&tok, tok->next);
		tok = skip(tok, "]");
		node = new_unary(ND_DEREF, new_add(node, idx, start), start);
	}
	*rest = tok;
	return node;
}

static Node *mul(Token **rest, Token *tok) {
	Node *node = unary(&tok, tok);
	for (;;) {
		Token *start = tok;
		if (equal(tok, "*")) {
			node = new_binary(ND_MUL, node, unary(&tok, tok->next), start);
		} else if (equal(tok, "/")) {
			node = new_binary(ND_DIV, node, unary(&tok, tok->next), start);
		} else {
			*rest = tok;
			return node;
		}
	}
}

// In C, `+` operator is overloaded to perform the pointer arithmetic.
// If p is a pointer, p+n adds not n but sizeof(*p)*n to the value of p,
// so that p+n points to the location n elements (not bytes) ahead of p.
// In other words, we need to scale an integer value before adding to a
// pointer value. This function takes care of the scaling.
static Node *new_add(Node *lhs, Node *rhs, Token *tok) {
  add_type(lhs);
  add_type(rhs);

  // num + num
  if (is_integer(lhs->ty) && is_integer(rhs->ty))
    return new_binary(ND_ADD, lhs, rhs, tok);

  // ptr + ptr (illegal)
  if (lhs->ty->base && rhs->ty->base)
    error_tok(tok, "invalid operands");

  // Canonicalize `num + ptr` to `ptr + num`.
  if (!lhs->ty->base && rhs->ty->base) {
    Node *tmp = lhs;
    lhs = rhs;
    rhs = tmp;
  }

  // ptr + num
  rhs = new_binary(ND_MUL, rhs, new_num(lhs->ty->base->size, tok), tok);
  rhs->pointer_arith = true;
  return new_binary(ND_ADD, lhs, rhs, tok);
}

// Like `+`, `-` is overloaded for the pointer type.
static Node *new_sub(Node *lhs, Node *rhs, Token *tok) {
  add_type(lhs);
  add_type(rhs);

  // num - num
  if (is_integer(lhs->ty) && is_integer(rhs->ty))
    return new_binary(ND_SUB, lhs, rhs, tok);

  // ptr - num
  if (lhs->ty->base && is_integer(rhs->ty)) {
    rhs = new_binary(ND_MUL, rhs, new_num(lhs->ty->base->size, tok), tok);
	rhs->pointer_arith = true;
    add_type(rhs);
    Node *node = new_binary(ND_SUB, lhs, rhs, tok);
    node->ty = lhs->ty;
    return node;
  }

  // ptr - ptr, which returns how many elements are between the two.
  if (lhs->ty->base && rhs->ty->base) {
    Node *node = new_binary(ND_SUB, lhs, rhs, tok);
    node->ty = ty_int;
	node = new_binary(ND_DIV, node, new_num(lhs->ty->base->size, tok), tok);
	node->pointer_arith = true;
    return node;
  }

  error_tok(tok, "invalid operands");
}

static Node *add(Token **rest, Token *tok) {
	Node *node = mul(&tok, tok);
	for (;;) {
		Token *start = tok;
		if (equal(tok, "+")) {
			node = new_add(node, mul(&tok, tok->next), start);
		} else if (equal(tok, "-")) {
			node = new_sub(node, mul(&tok, tok->next), start);
		} else {
			*rest = tok;
			return node;
		}
	}
}

static Node *relational(Token **rest, Token *tok) {
	Node *node = add(&tok, tok);
	for (;;) {
		Token *start = tok;
		if (equal(tok, "<")) {
			node = new_binary(ND_LT, node, add(&tok, tok->next), start);
		} else if (equal(tok, "<=")) {
			node = new_binary(ND_LE, node, add(&tok, tok->next), start);
		} else if (equal(tok, ">")) {
			node = new_binary(ND_LT, add(&tok, tok->next), node, start);
		} else if (equal(tok, ">=")) {
			node = new_binary(ND_LE, add(&tok, tok->next), node, start);
		} else {
			*rest = tok;
			return node;
		}
	}
}

static Node *equality(Token **rest, Token *tok) {
	Node *node = relational(&tok, tok);
	for (;;) {
		Token *start = tok;
		if (equal(tok, "==")) {
			// fprintf(stderr, "OUTPUT EQU !!\n");
			node = new_binary(ND_EQ, node, relational(&tok, tok->next), start);
		} else if (equal(tok, "!=")) {
			node = new_binary(ND_NE, node, relational(&tok, tok->next), start);
		} else {
			*rest = tok;
			return node;
		}
	}
}

static Node *assign(Token **rest, Token *tok) {
	Node *node = equality(&tok, tok);
	if (equal(tok, "=")) {
		// fprintf(stderr, "OUTPUT ASSIGN !!\n");
		node = new_binary(ND_ASSIGN, node, assign(&tok, tok->next), tok);
	}
	*rest = tok;
	return node;
}

static Node *expr(Token **rest, Token *tok) {
	return assign(rest, tok);
}

static Node *expr_stmt(Token **rest, Token *tok) {
  if (equal(tok, ";")) {
    *rest = tok->next;
    return new_node(ND_BLOCK, tok);
  }

  Node *node = new_node(ND_EXPR_STMT, tok);
  node->lhs = expr(&tok, tok);
  *rest = skip(tok, ";");
  return node;
}

static Node *compound_stmt(Token **rest, Token *tok) {
  Node *node = new_node(ND_BLOCK, tok);

  Node head = {};
  Node *cur = &head;
  while (!equal(tok, "}")) {
    if (equal(tok, "int") || equal(tok, "long"))
      cur = cur->next = declaration(&tok, tok);
    else
      cur = cur->next = stmt(&tok, tok);
    add_type(cur);
  }

  node->body = head.next;
  *rest = tok->next;
  return node;
}

static Node *declaration(Token **rest, Token *tok) {
  Type *basety = declspec(&tok, tok);

  Node head = {};
  Node *cur = &head;
  int i = 0;

  while (!equal(tok, ";")) {
    if (i++ > 0)
      tok = skip(tok, ",");

    Type *ty = declarator(&tok, tok, basety);
    Obj *var = new_lvar(get_ident(ty->name), ty);

    if (!equal(tok, "="))
      continue;

    Node *lhs = new_var_node(var, ty->name);
    Node *rhs = assign(&tok, tok->next);
    Node *node = new_binary(ND_ASSIGN, lhs, rhs, tok);
    cur = cur->next = new_unary(ND_EXPR_STMT, node, tok);
  }

  Node *node = new_node(ND_BLOCK, tok);
  node->body = head.next;
  *rest = tok->next;
  return node;
}

static Node *stmt(Token **rest, Token *tok) {
	if (equal(tok, "return")) {
		Node *node = new_node(ND_RETURN, tok);
		node->lhs = expr(&tok, tok->next);
		*rest = skip(tok, ";");
		return node;
	} else if (equal(tok, "if")) {
		Node *node = new_node(ND_IF, tok);
		tok = skip(tok->next, "(");
		node->cond = expr(&tok, tok);
		tok = skip(tok, ")");
		node->then = stmt(&tok, tok);
		if (equal(tok, "else")) {
			node->els = stmt(&tok, tok->next);
		}
		*rest = tok;
		return node;
	} else if (equal(tok, "for")) {
		Node *node = new_node(ND_FOR, tok);
		tok = skip(tok->next, "(");
		node->init = expr_stmt(&tok, tok);
		if (!equal(tok, ";")) {
			node->cond = expr(&tok, tok);
		}
		tok = skip(tok, ";");
		if (!equal(tok, ")")) {
			node->inc = expr(&tok, tok);
		}
		tok = skip(tok, ")");
		node->then = stmt(rest, tok);
		return node;
	}
	else if (equal(tok, "while")) {
		Node *node = new_node(ND_FOR, tok);
		tok = skip(tok->next, "(");
		node->cond = expr(&tok, tok);
		tok = skip(tok, ")");
		node->then = stmt(rest, tok);
		return node;
	}
	if (equal(tok, "{")) {
		return compound_stmt(rest, tok->next);
	}
	return expr_stmt(rest, tok);
}

static void create_param_lvars(Type *param) {
  if (param) {
    create_param_lvars(param->next);
    new_lvar(get_ident(param->name), param);
  }
}

static Token *function(Token *tok, Type *basety) {
  Type *ty = declarator(&tok, tok, basety);

  Obj *fn = new_gvar(get_ident(ty->name), ty);
  fn->is_function = true;

  locals = NULL;
  create_param_lvars(ty->params);
  fn->params = locals;

  tok = skip(tok, "{");
  fn->body = compound_stmt(&tok, tok);
  fn->locals = locals;
  return tok;
}

static Token *global_variable(Token *tok, Type *basety) {
  bool first = true;

  while (!consume(&tok, tok, ";")) {
    if (!first)
      tok = skip(tok, ",");
    first = false;

    Type *ty = declarator(&tok, tok, basety);
    new_gvar(get_ident(ty->name), ty);
  }
  return tok;
}

// Lookahead tokens and returns true if a given token is a start
// of a function definition or declaration.
static bool is_function(Token *tok) {
  if (equal(tok, ";"))
    return false;

  Type dummy = {};
  Type *ty = declarator(&tok, tok, &dummy);
  return ty->kind == TY_FUNC;
}

Obj *parse(Token *tok) {
	globals = 0;
	char *env = getenv("PRINT_AST");
	if (env) {
		sscanf(env, "%d", &do_print_ast);
	}
	// if (do_print_ast)
	// 	fprintf(stderr, "Will print AST..\n");
	while (tok->kind != TK_EOF) {
		Type *basety = declspec(&tok, tok);
		if (is_function(tok)) {
			tok = function(tok, basety);
			continue;
		}
		tok = global_variable(tok, basety);
	}
	if (do_print_ast) {
		print_obj(globals, 0);
	}
	return globals;
}
