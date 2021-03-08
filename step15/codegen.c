#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "9cc.h"

void gen_lval(Node *node) {
	if (node->kind != ND_LVAR) {
		error("Left side value of assignment is not a variable");
	}
	printf("\tmov rax, rbp\n");
	printf("\tsub rax, %d\n", node->offset);
	printf("\tpush rax\n		// End of gen_lval node=%p\n", node);
}

void gen(Node *node) {
	if (!node) {
		return;
	}
	switch (node->kind) {
		case ND_CALL:
		{
			LVar *lvar = (LVar *)node->lhs;
			char buf[1000];
			snprintf(buf, lvar->len + 1, "%s", lvar->name);
			// printf("\tpop \n", buf);
			Node *next = node->rhs;
			int nargs = 0;
			while (next) {
				gen(next);
				next = next->next;
				// fprintf(stderr, "ARG!\n");
				nargs++;
			}
			if (nargs>=2) {
				printf("\tpop rsi\n");
			}
			if (nargs>=1) {
				printf("\tpop rdi\n");
			}
			// Following RSP 16-byte adjustment is important to respect x64 ABI
			printf("\tand rsp, 0xfffffffffffffff0\n");
			printf("\tcall %s\n", buf);
			printf("\tpush rax\n			// End of call\n");
			return;
		}
		case ND_BLOCK:
		{
			Node *next = node->next;
			while (next) {
				gen(next);
				printf("\tpop rax\n");
				next = next->next;
			}
			return;
		}
		case ND_FOR:
			printf("// FOR%d n=%p l=%p r=%p", node->offset, node->next, node->lhs, node->rhs);
			if (node->lhs)
				printf(" ll=%p lr=%p", node->lhs->lhs, node->lhs->rhs);
			if (node->rhs)
				printf(" rl=%p rr=%p", node->rhs->lhs, node->rhs->rhs);
			printf("\n");
			gen(node->lhs);
			printf("\t.Lforbegin%d:\n", node->offset);
			if (node->rhs->lhs) {
				gen(node->rhs->lhs);
				printf("\tpop rax\n");
				printf("\tcmp rax, 0\n");
				printf("\tje .Lforend%d\n", node->offset);
			}
			printf("// gen rhs\n");
			gen(node->rhs->rhs->rhs);
			printf("// gen lhs\n");
			gen(node->rhs->rhs->lhs);
			printf("\tjmp .Lforbegin%d\n", node->offset);
			printf("\t.Lforend%d:\n", node->offset);
			return;
		case ND_WHILE:
			printf("\t.Lwhilebegin%d:\n", node->offset);
			break;
		case ND_NUM:
			printf("\tpush %d\n", node->val);
			return;
		case ND_LVAR:
			gen_lval(node);
			printf("\tpop rax\n");
			printf("\tmov rax, [rax]\n");
			printf("\tpush rax\n		// End of lvar\n");
			return;
		case ND_ASSIGN:
			gen_lval(node->lhs);
			gen(node->rhs);
			printf("\tpop rdi\n");
			printf("\tpop rax\n");
			printf("\tmov [rax], rdi\n");
			printf("\tpush rdi\n");
			return;
	}
	gen(node->lhs);
	if (node->kind == ND_RETURN) {
		printf("\tpop rax\n");
		printf("\tmov rsp,rbp\n");
		printf("\tpop rbp\n");
		printf("\tret\n");
		return;
	} else if (node->kind == ND_IF) {
		printf("\tpop rax\n");
		printf("\tcmp rax, 0\n");
		if (node->rhs->kind == ND_ELSE) {
			printf("\tje .Lelse%d\n", node->offset);
		} else {
			printf("\tje .Lifend%d\n", node->offset);
		}
	} else if (node->kind == ND_ELSE) {
		printf("\tjmp .Lifend%d\n", node->offset);
		printf("\t.Lelse%d:\n", node->offset);
	} else if (node->kind == ND_WHILE) {
		printf("\tpop rax\n");
		printf("\tcmp rax, 0\n");
		printf("\tje .Lwhileend%d\n", node->offset);
	}
	gen(node->rhs);
	if (node->kind == ND_WHILE) {
		printf("\tjmp .Lwhilebegin%d\n", node->offset);
	}
	if (node->kind == ND_IF) {
		printf("\t.Lifend%d:\n", node->offset);
	}
	if (node->kind == ND_WHILE) {
		printf("\t.Lwhileend%d:\n", node->offset);
	}
	if (node->kind == ND_IF || node->kind == ND_ELSE || node->kind == ND_WHILE) {
		return;
	}
	printf("\tpop rdi\n");
	printf("\tpop rax\n");
	switch (node->kind) {
		case ND_EQU:
			printf("\tcmp rax, rdi\n");
			printf("\tsete al\n");
			printf("\tmovzb rax, al\n");
			break;
		case ND_EQUNOT:
			printf("\tcmp rax, rdi\n");
			printf("\tsetne al\n");
			printf("\tmovzb rax, al\n");
			break;
		case ND_RELINF:
			printf("\tcmp rax, rdi\n");
			printf("\tsetl al\n");
			printf("\tmovzb rax, al\n");
			break;
		case ND_RELINFEQ:
			printf("\tcmp rax, rdi\n");
			printf("\tsetle al\n");
			printf("\tmovzb rax, al\n");
			break;
		case ND_RELSUP:
			printf("\tcmp rdi, rax\n");
			printf("\tsetl al\n");
			printf("\tmovzb rax, al\n");
			break;
		case ND_RELSUPEQ:
			printf("\tcmp rdi, rax\n");
			printf("\tsetle al\n");
			printf("\tmovzb rax, al\n");
			break;
		case ND_ADD:
			printf("\tadd rax,rdi\n");
			break;
		case ND_SUB:
			printf("\tsub rax,rdi\n");
			break;
		case ND_MUL:
			printf("\timul rax,rdi\n");
			break;
		case ND_DIV:
			printf("\tcqo\n");
			printf("\tidiv rdi\n");
			break;
		default:
			error("%s: Unknown node type %d\n", __func__, node->kind);
	}
	printf("\tpush rax\n			// End of gen\n");
}

Node *code;
void generate() {
	printf(".intel_syntax noprefix\n");
	printf(".globl main\n");
	printf("\n");
	printf("main:\n");
	printf("\tpush rbp\n");
	printf("\tmov rbp, rsp\n");
	printf("\tsub rsp, 208\n");
	Node *next = code;
	while (next) {
		gen(next);
		printf("\tpop rax\n");
		next = next->next;
	}
	printf("\tmov rsp,rbp\n");
	printf("\tpop rbp\n");
	printf("\tret\n");
}
