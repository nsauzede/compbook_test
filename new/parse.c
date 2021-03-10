#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "9cc.h"

LVar g_empty_local;
LVar *locals = &g_empty_local;

void print_node(Node *node, int depth) {
	if (!node) {
		return;
	}
	fprintf(stderr, "%*s", depth, "");
	if (node->kind == ND_CALL) {
		LVar *lvar = (LVar *)node->lhs;
		char buf[1000];
		snprintf(buf, lvar->len + 1, "%s", lvar->name);
		fprintf(stderr, "CALL %s()\n", buf);
		return;
	}
	print_node(node->lhs, depth);
	switch (node->kind) {
		case ND_BLOCK:
		{
			Node *next = node->next;
			fprintf(stderr, "BLOCK\n");
			while (next) {
				print_node(next, depth + 1);
				// fprintf(stderr, ";");
				next = next->next;
			}
			// fprintf(stderr, "}\n");
			break;
		}
		case ND_RETURN:fprintf(stderr, "RETURN\n");break;
		case ND_IF:fprintf(stderr, "IF\n");break;
		case ND_ELSE:fprintf(stderr, "ELSE\n");break;
		case ND_WHILE:fprintf(stderr, "WHILE\n");break;
		case ND_FOR:fprintf(stderr, "FOR\n");break;
		case ND_EQU:fprintf(stderr, "EQU\n");break;
		case ND_LVAR:fprintf(stderr, "LVAR\n");break;
		case ND_NUM:fprintf(stderr, "NUM %d\n", node->val);break;
		case ND_ASSIGN:fprintf(stderr, "ASSIGN\n");break;
		case ND_ADD:fprintf(stderr, "ADD\n");break;
		case ND_RELINF:fprintf(stderr, "INF\n");break;
		default:fprintf(stderr, "Whaat about node %d ??\n", node->kind);break;
	}
	print_node(node->rhs, depth);
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
		print_node(*nodes, 0);
		nodes++;
	}
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
		node->paren = true;
		return node;
	}
	Token *tok = consume_ident();
	if (tok) {
		if (consume("(")) {
			Node *arg = 0;
			Node **next_arg = &arg;
			bool first = true;
			while (1) {
				if (consume(")")) {
					break;
				} else if (!first) {
					expect(",");
				}
				*next_arg = expr();
				// fprintf(stderr, "ADDED ARG\n");
				next_arg = &(*next_arg)->next;
				first = false;
			}
			LVar *lvar = calloc(1, sizeof(LVar));
			// fprintf(stderr, "ADDED CALL\n");
			Node *node = new_node(ND_CALL, (Node *)lvar, arg);
			lvar->name = tok->str;
			lvar->len = tok->len;
			return node;

		node = new_node(ND_BLOCK, 0, 0);
		Node **next = &node->next;
		while (1) {
			Node *stmt_ = stmt();
			if (!stmt_) {
				break;
			}
			// fprintf(stderr, "ADDED STMT TO LIST\n");
			*next = stmt_;
			next = &stmt_->next;
		}
		return node;
		}
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
			lvar->offset = locals->offset + 1;
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
	static int label = 0;
	bool is_return = false;
	bool is_if = false;
	bool is_while = false;
	Node *node;
	if (consume("}")) {
		return 0;
	}
	if (consume("{")) {
		node = new_node(ND_BLOCK, 0, 0);
		Node **next = &node->next;
		while (1) {
			Node *stmt_ = stmt();
			if (!stmt_) {
				break;
			}
			// fprintf(stderr, "ADDED STMT TO LIST\n");
			*next = stmt_;
			next = &stmt_->next;
		}
		return node;
	}
	if (consume_keyword("if")) {
		expect("(");
		is_if = true;
	}
	else if (consume_keyword("while")) {
		expect("(");
		is_while = true;
	}
	else if (consume_keyword("for")) {
		expect("(");
		Node *expr1 = 0, *expr2 = 0, *expr3 = 0;
		if (!consume(";")) {
			expr1 = expr();
			expect(";");
		}
		if (!consume(";")) {
			expr2 = expr();
			expect(";");
		}
		if (!consume(")")) {
			expr3 = expr();
			expect(")");
		}
		node = new_node(ND_FOR, expr3, stmt());
		node->offset = label;
		node = new_node(ND_FOR, expr2, node);
		node->offset = label;
		node = new_node(ND_FOR, expr1, node);
		node->offset = label;
		label++;
		return node;
	}
	else if (consume_keyword("return")) {
		is_return = true;
	}
	node = expr();
	if (is_if) {
		expect(")");
		Node *then_ = stmt();
		if (consume_keyword("else")) {
			then_ = new_node(ND_ELSE, then_, stmt());
			then_->offset = label;
		}
		node = new_node(ND_IF, node, then_);
		node->offset = label;
		label++;
		return node;
	} else if (is_while) {
		expect(")");
		node = new_node(ND_WHILE, node, stmt());
		node->offset = label;
		label++;
		return node;
	} else if (is_return) {
		// fprintf(stderr, "OUTPUT RETURN !!\n");
		node = new_node(ND_RETURN, node, NULL);
	}
	expect(";");
	return node;
}

Node *code[100];
void body() {
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

void func() {
	body();
}

void program() {
	func();
}

void parse() {
	program();
}
