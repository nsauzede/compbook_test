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
	printf("\tpush rax\n");
}

void gen(Node *node) {
	if (!node) {
		return;
	}
	switch (node->kind) {
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
			gen(node->lhs);
			printf("\t.Lbegin%d:\n", node->offset);
			gen(node->rhs->lhs);
			printf("\tpop rax\n");
			printf("\tcmp rax, 0\n");
			printf("\tje .Lend%d\n", node->offset);
			gen(node->rhs->rhs->rhs);
			gen(node->rhs->rhs->lhs);
			printf("\tjmp .Lbegin%d\n", node->offset);
			printf("\t.Lend%d:\n", node->offset);
			return;
		case ND_WHILE:
			printf("\t.Lbegin%d:\n", node->offset);
			break;
		case ND_NUM:
			printf("\tpush %d\n", node->val);
			return;
		case ND_LVAR:
			gen_lval(node);
			printf("\tpop rax\n");
			printf("\tmov rax, [rax]\n");
			printf("\tpush rax\n");
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
			printf("\tje .Lend%d\n", node->offset);
		}
	} else if (node->kind == ND_ELSE) {
		printf("\tjmp .Lend%d\n", node->offset);
		printf("\t.Lelse%d:\n", node->offset);
	} else if (node->kind == ND_WHILE) {
		printf("\tpop rax\n");
		printf("\tcmp rax, 0\n");
		printf("\tje .Lend%d\n", node->offset);
	}
	gen(node->rhs);
	if (node->kind == ND_WHILE) {
		printf("\tjmp .Lbegin%d\n", node->offset);
	}
	if (node->kind == ND_IF || node->kind == ND_WHILE) {
		printf("\t.Lend%d:\n", node->offset);
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
			error("Unknown node type %d\n", node->kind);
	}
	printf("\tpush rax\n");
}

Node *code[100];
void generate() {
	printf(".intel_syntax noprefix\n");
	printf(".globl main\n");
	printf("\n");
	printf("main:\n");
	printf("\tpush rbp\n");
	printf("\tmov rbp, rsp\n");
	printf("\tsub rsp, 208\n");
	for (int i = 0; code[i]; i++) {
		gen(code[i]);
		printf("\tpop rax\n");
	}
	printf("\tmov rsp,rbp\n");
	printf("\tpop rbp\n");
	printf("\tret\n");
}
