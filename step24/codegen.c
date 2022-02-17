#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "chibicc.h"

static Obj *current_fn;
static char *argreg[] = {"%rdi", "%rsi", "%rdx", "%rcx", "%r8", "%r9"};
static int depth;

static void gen_expr(Node *node);

static int count() {
	static int i = 1;
	return i++;
}

static void push(void) {
  printf("\tpush %%rax\n");
  depth++;
}
  
static void pop(char *arg) {
  printf("\tpop %s\n", arg);
  depth--;
}

static int align_to(int n, int align) {
  return (n + align - 1) / align * align;
}

static void gen_addr(Node *node) {
  switch (node->kind) {
  case ND_VAR:
    if (node->var->is_local) {
      // Local variable
      printf("\tlea %d(%%rbp), %%rax\n", node->var->offset);
    } else {
      // Global variable
      printf("\tlea %s(%%rip), %%rax\n", node->var->name);
    }
    return;
  case ND_DEREF:
    gen_expr(node->lhs);
    return;
  }

  error_tok(node->tok, "not an lvalue");
}

// Load a value from where %rax is pointing to.
static void load(Node *node) {
if (node->ty->kind == TY_ARRAY) {
    // If it is an array, do not attempt to load a value to the
    // register because in general we can't load an entire array to a
    // register. As a result, the result of an evaluation of an array
    // becomes not the array itself but the address of the array.
    // This is where "array is automatically converted to a pointer to
    // the first element of the array in C" occurs.
    return;
  }
	switch (node->ty->size) {
	case 8:
		printf("\tmov (%%rax), %%rax\n");
		break;
	case 4:
		printf("\tmov (%%rax), %%eax\n");
		break;
	default:
		error_tok(node->tok, "unsupported load size %d", node->ty->size);
	}
}

// Store %rax to an address that the stack top is pointing to.
static void store(Node *node) {
	pop("%rdi");
	switch (node->ty->size) {
	case 8:
		printf("\tmov %%rax, (%%rdi)\n");
		break;
	case 4:
		printf("\tmov %%eax, (%%rdi)\n");
		break;
	default:
		error_tok(node->tok, "unsupported store size %d", node->ty->size);
	}
}

static void gen_expr(Node *node) {
	switch (node->kind) {
		case ND_NUM:
			printf("\tmov $%ld, %%rax\n", node->val);
			return;
		case ND_NEG:
			gen_expr(node->lhs);
			printf("\tneg %%rax\n");
			return;
		case ND_VAR:
			gen_addr(node);
			load(node);
			return;
		case ND_DEREF:
			gen_expr(node->lhs);
			load(node);
			return;
		case ND_ADDR:
			gen_addr(node->lhs);
			return;
		case ND_ASSIGN:
			gen_addr(node->lhs);
			push();
			gen_expr(node->rhs);
			store(node);
			return;
		case ND_FUNCALL: {
			int nargs = 0;
			for (Node *arg = node->args; arg; arg = arg->next) {
				gen_expr(arg);
				push();
				nargs++;
			}
			for (int i = nargs - 1; i >= 0; i--) {
				pop(argreg[i]);
			}
			// Following RSP 16-byte adjustment is important to respect x64 ABI
			// printf("\tand rsp, 0xfffffffffffffff0\n");
			printf("\tmov $0, %%rax\n");
			printf("\tcall %s\n", node->funcname);
			return;
		}
	}
	gen_expr(node->rhs);
	push();
	gen_expr(node->lhs);
	pop("%rdi");
	switch (node->kind) {
		case ND_ADD:
			printf("\tadd %%rdi, %%rax\n");
			return;
		case ND_SUB:
			printf("\tsub %%rdi, %%rax\n");
			return;
		case ND_MUL:
			printf("\timul %%rdi, %%rax\n");
			return;
		case ND_DIV:
			printf("\tcqo\n");
			printf("\tidiv %%rdi\n");
			return;
		case ND_EQ:
		case ND_NE:
		case ND_LT:
		case ND_LE:
			printf("\tcmp %%rdi, %%rax\n");
			if (node->kind == ND_EQ)
				printf("\tsete %%al\n");
			else if (node->kind == ND_NE)
				printf("\tsetne %%al\n");
			else if (node->kind == ND_LT)
				printf("\tsetl %%al\n");
			else if (node->kind == ND_LE)
				printf("\tsetle %%al\n");
			printf("\tmovzb %%al, %%rax\n");
			return;
	}
	error_tok(node->tok, "invalid expression");
}

static void gen_stmt(Node *node) {
	switch (node->kind) {
		case ND_IF: {
			int c = count();
			gen_expr(node->cond);
			printf("\tcmp $0, %%rax\n");
			printf("\tje .L.else.%d\n", c);
			gen_stmt(node->then);
			printf("\tjmp .L.end.%d\n", c);
			printf(".L.else.%d:\n", c);
			if (node->els)
				gen_stmt(node->els);
			printf(".L.end.%d:\n", c);
			return;
		}
		case ND_FOR: {
			int c = count();
			if (node->init)
				gen_stmt(node->init);
			printf(".L.begin.%d:\n", c);
			if (node->cond) {
				gen_expr(node->cond);
				printf("\tcmp $0, %%rax\n");
				printf("\tje .L.end.%d\n", c);
			}
			gen_stmt(node->then);
			if (node->inc)
				gen_expr(node->inc);
			printf("\tjmp .L.begin.%d\n", c);
			printf(".L.end.%d:\n", c);
			return;
		}
		case ND_BLOCK:
			for (Node *n = node->body; n; n = n->next)
				gen_stmt(n);
			return;
		case ND_RETURN:
			gen_expr(node->lhs);
			printf("\tjmp .L.return.%s\n", current_fn->name);
			return;
		case ND_EXPR_STMT:
			gen_expr(node->lhs);
			return;
	}
	error_tok(node->tok, "invalid statement");
}

static void assign_lvar_offsets(Obj *prog) {
  for (Obj *fn = prog; fn; fn = fn->next) {
    if (!fn->is_function)
      continue;

    int offset = 0;
    for (Obj *var = fn->locals; var; var = var->next) {
      offset += var->ty->size;
      var->offset = -offset;
    }
    fn->stack_size = align_to(offset, 16);
		// Reverse offsets because we created lvars backward
		for (Obj *var = fn->locals; var; var = var->next) {
			var->offset = -fn->stack_size-var->offset-var->ty->size;
		}
  }
}

static void emit_data(Obj *prog) {
	for (Obj *obj = prog; obj; obj = obj->next) {
		if (obj->is_function) {
			continue;
		}
		printf("\t.data\n");
		printf("\t.globl %s\n", obj->name);
		printf("%s:\n", obj->name);
		printf("\t.zero %d\n", obj->ty->size);
	}
}

static void emit_text(Obj *prog) {
	for (Obj *obj = prog; obj; obj = obj->next) {
		if (!obj->is_function) {
			continue;
		}
		printf("\t.globl %s\n", obj->name);
		printf("\t.text\n");
		printf("%s:\n", obj->name);
		current_fn = obj;
		// prologue
		printf("\tpush %%rbp\n");
		printf("\tmov %%rsp, %%rbp\n");
		printf("\tsub $%d, %%rsp\n", obj->stack_size);
		// save passed-by-registers args to the stack
		int i = 0;
		for (Obj *var = obj->params; var; var = var->next) {
			printf("\tmov %s, %%rax\n", argreg[i++]);
			switch (var->ty->size) {
			case 8:
				printf("\tmov %%rax, %d(%%rbp)\n", var->offset);
				break;
			case 4:
				printf("\tmov %%eax, %d(%%rbp)\n", var->offset);
				break;
			default:
				fprintf(stderr, "unsupported passed-by-register arg size %d\n", var->ty->size);
				exit(1);
			}
		}
		// emit code
		gen_stmt(obj->body);
		assert(depth == 0);
		// epilogue
		printf(".L.return.%s:\n", obj->name);
		printf("\tmov %%rbp,%%rsp\n");
		printf("\tpop %%rbp\n");
		printf("\tret\n");
	}
}

void codegen(Obj *prog, char *input) {
	int gen_v = 0;
	char *env = getenv("GEN_V");
	if (env) {
		sscanf(env, "%d", &gen_v);
	}
	if (gen_v) {
		extern void codegen_v(Obj *prog, char *input);
		codegen_v(prog, input);
		return;
	}
	assign_lvar_offsets(prog);
	emit_data(prog);
	emit_text(prog);
}
