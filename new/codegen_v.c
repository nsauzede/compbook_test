#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "9cc.h"

static void gen_lval(Node *node) {
	if (node->kind != ND_LVAR) {
		error("Left side value of assignment is not a variable");
	}
	// printf("\n/*hello*/\n");
	printf("lvar%d", node->offset);
}

static void gen(Node *node) {
	if (!node) {
		return;
	}
	if (node->paren) {
		printf("(");
	}
	bool is_return = false;
	bool is_assign = false;
	bool is_if = false;
	bool is_else = false;
	bool is_while = false;
	bool cast_int_to_bool = false;
	switch (node->kind) {
		case ND_CALL:
		{
			LVar *lvar = (LVar *)node->lhs;
			char buf[1000];
			snprintf(buf, lvar->len + 1, "%s", lvar->name);
			printf("%s(", buf);
			Node *next = node->rhs;
			int nargs = 0;
			while (next) {
				gen(next);
				next = next->next;
				// fprintf(stderr, "ARG!\n");
				nargs++;
			}
			printf(")");
			return;
		}
		case ND_BLOCK:
		{
			Node *next = node->next;
			// printf("/*begin block*/\n");
			while (next) {
				gen(next);
				// printf("\tpop rax\n");
				next = next->next;
			}
			// printf("/*end block*/\n");
			return;
		}
		case ND_FOR:
			printf("for ");
			gen(node->lhs);
			printf(";");
			gen(node->rhs->lhs);
			printf(";");
			gen(node->rhs->rhs->lhs);
			printf("{\n");
			gen(node->rhs->rhs->rhs);
			printf("}/*end for*/\n");
			return;
		case ND_IF:
			printf("if ");
			cast_int_to_bool = true;
			is_if = true;
			break;
		case ND_ELSE:
			is_else = true;
			break;
		case ND_WHILE:
			printf("for ");
			is_while = true;
			break;
		case ND_NUM:
			printf("%d", node->val);
			return;
		case ND_LVAR:
			gen_lval(node);
			return;
		case ND_ASSIGN:
			gen_lval(node->lhs);
			printf(" = ");
			gen(node->rhs);
			printf("\n");
			return;
		case ND_RETURN:
			// printf("/*RETURN !!*/");
			is_return = true;
			break;
		case ND_RELINF:
			// printf("/*RELINF BOOL*/");
		case ND_EQU:
		case ND_EQUNOT:
		case ND_RELINFEQ:
		case ND_RELSUP:
		case ND_RELSUPEQ:
			node->boolean = true;
			break;
	}
	if (is_return) {
		printf("exit(int(");	// hack, until proper functions support
	}
	if (cast_int_to_bool && !node->lhs->boolean) {
		printf("0!=(");		// open the int-to-bool cast paren
	}
	// printf("\n/*did we miss a return ? lhs*/\n");
	gen(node->lhs);
	if (cast_int_to_bool && !node->lhs->boolean) {
		printf(")");		// close the int-to-bool cast paren
	}
	if (is_if || is_while) {
		printf("{");
	}

	switch (node->kind) {
		case ND_EQU:
			printf("==");
			break;
		case ND_EQUNOT:
			printf("!=");
			break;
		case ND_RELINF:
			printf("<");
			break;
		case ND_RELINFEQ:
			printf("<=");
			break;
		case ND_RELSUP:
			printf(">");
			break;
		case ND_RELSUPEQ:
			printf(">=");
			break;
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
		// default:
		// 	error("%s: Unknown node type %d\n", __func__, node->kind);
	}

	if (is_if && !node->rhs->rhs) {
		printf("}");
	}

	if (is_return) {
		printf("))");
		return;
	} else if (node->kind == ND_IF) {
		// printf("/*if*/");
		if (node->rhs->kind == ND_ELSE) {
			// printf("/*ifelse*/");
		} else {
			// printf("/*ifnoteelse*/");
		}
	} else if (is_else) {
		printf("} else {");
	} else if (node->kind == ND_WHILE) {
		// printf("\tpop rax\n");
		// printf("\tcmp rax, 0\n");
		// printf("\tje .Lend%d\n", node->offset);
	}
	gen(node->rhs);

	if (is_else || is_while) {
		printf("}");
	}

	if (node->kind == ND_WHILE) {
		// printf("\tjmp .Lbegin%d\n", node->offset);
	}
	if (node->kind == ND_IF || node->kind == ND_WHILE) {
		// printf("\t.Lend%d:\n", node->offset);
	}
	if (node->kind == ND_IF || node->kind == ND_ELSE || node->kind == ND_WHILE) {
		return;
	}

	//printf("\tpop rdi\n");
	//printf("\tpop rax\n");
	//printf("\tpush rax\n");
	if (node->paren) {
		printf(")");
	}
}

void generate_v() {
	LVar *local = locals;
	while (local) {
		if (local->len) {
			char buf[1000];
			snprintf(buf, local->len + 1, "%s", local->name);
			printf("mut lvar%d := 0	// %s %d\n", local->offset, buf, local->offset);
		}
		local = local->next;
	}
	// printf("mut ret:=0\n");
	// printf("ret=");
	for (int i = 0; code[i]; i++) {
		gen(code[i]);
		//printf("\tpop rax\n");
	}
	printf("\n");
	//printf("exit(ret)\n");
}
