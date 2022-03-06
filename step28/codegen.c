#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "chibicc.h"

static char *assembly = 0;
static size_t assembly_len = 0;
static FILE *assembly_file = 0;
#define PRINTF(...) do{fprintf(assembly_file,__VA_ARGS__);}while(0)

static Obj *current_fn;
static char *argreg[] = {"%rdi", "%rsi", "%rdx", "%rcx", "%r8", "%r9"};
static int depth;

static void gen_expr(Node *node);
static void gen_stmt(Node *node);

static int count() {
	static int i = 1;
	return i++;
}

static void push(void) {
  PRINTF("\tpush %%rax\n");
  depth++;
}
  
static void pop(char *arg) {
  PRINTF("\tpop %s\n", arg);
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
      PRINTF("\tlea %d(%%rbp), %%rax\n", node->var->offset);
    } else {
      // Global variable
      PRINTF("\tlea %s(%%rip), %%rax\n", node->var->name);
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
		PRINTF("\tmov (%%rax), %%rax\n");
		break;
	case 4:
		PRINTF("\tmovsxd (%%rax), %%rax\n");
		break;
	case 1:
		PRINTF("\tmovsbl (%%rax), %%eax\n");
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
		PRINTF("\tmov %%rax, (%%rdi)\n");
		break;
	case 4:
		PRINTF("\tmov %%eax, (%%rdi)\n");
		break;
	case 1:
		PRINTF("\tmov %%al, (%%rdi)\n");
		break;
	default:
		error_tok(node->tok, "unsupported store size %d", node->ty->size);
	}
}

static void gen_expr(Node *node) {
	switch (node->kind) {
		case ND_NUM:
			PRINTF("\tmov $%ld, %%rax\n", node->val);
			return;
		case ND_NEG:
			gen_expr(node->lhs);
			PRINTF("\tneg %%rax\n");
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
		case ND_STMT_EXPR:
			for (Node *n = node->body; n; n = n->next) {
				gen_stmt(n);
			}
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
			// PRINTF("\tand rsp, 0xfffffffffffffff0\n");
			PRINTF("\tmov $0, %%rax\n");
			PRINTF("\tcall %s\n", node->funcname);
			return;
		}
	}
	gen_expr(node->rhs);
	push();
	gen_expr(node->lhs);
	pop("%rdi");
	switch (node->kind) {
		case ND_ADD:
			PRINTF("\tadd %%rdi, %%rax\n");
			return;
		case ND_SUB:
			PRINTF("\tsub %%rdi, %%rax\n");
			return;
		case ND_MUL:
			PRINTF("\timul %%rdi, %%rax\n");
			return;
		case ND_DIV:
			PRINTF("\tcqo\n");
			PRINTF("\tidiv %%rdi\n");
			return;
		case ND_EQ:
		case ND_NE:
		case ND_LT:
		case ND_LE:
			PRINTF("\tcmp %%rdi, %%rax\n");
			if (node->kind == ND_EQ)
				PRINTF("\tsete %%al\n");
			else if (node->kind == ND_NE)
				PRINTF("\tsetne %%al\n");
			else if (node->kind == ND_LT)
				PRINTF("\tsetl %%al\n");
			else if (node->kind == ND_LE)
				PRINTF("\tsetle %%al\n");
			PRINTF("\tmovzb %%al, %%rax\n");
			return;
	}
	error_tok(node->tok, "invalid expression");
}

static void gen_stmt(Node *node) {
	switch (node->kind) {
		case ND_IF: {
			int c = count();
			gen_expr(node->cond);
			PRINTF("\tcmp $0, %%rax\n");
			PRINTF("\tje .L.else.%d\n", c);
			gen_stmt(node->then);
			PRINTF("\tjmp .L.end.%d\n", c);
			PRINTF(".L.else.%d:\n", c);
			if (node->els)
				gen_stmt(node->els);
			PRINTF(".L.end.%d:\n", c);
			return;
		}
		case ND_FOR: {
			int c = count();
			if (node->init)
				gen_stmt(node->init);
			PRINTF(".L.begin.%d:\n", c);
			if (node->cond) {
				gen_expr(node->cond);
				PRINTF("\tcmp $0, %%rax\n");
				PRINTF("\tje .L.end.%d\n", c);
			}
			gen_stmt(node->then);
			if (node->inc)
				gen_expr(node->inc);
			PRINTF("\tjmp .L.begin.%d\n", c);
			PRINTF(".L.end.%d:\n", c);
			return;
		}
		case ND_BLOCK:
			for (Node *n = node->body; n; n = n->next)
				gen_stmt(n);
			return;
		case ND_RETURN:
			gen_expr(node->lhs);
			PRINTF("\tjmp .L.return.%s\n", current_fn->name);
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
		PRINTF("\t.data\n");
		PRINTF("\t.globl %s\n", obj->name);
		PRINTF("%s:\n", obj->name);
		if (obj->init_data) {
			PRINTF("\t.byte ");
			int backslash = 0;
			int first = 1;
			for (int i = 0; i < obj->ty->size; i++) {
				unsigned char c = ((unsigned char *)obj->init_data)[i];
				if (backslash) {
					if (c == 'x') {
						fprintf(stderr, "hex escape unsupported\n");
						exit(1);
					} else if (c >= '0' && c <= '7') {
						fprintf(stderr, "octal escape unsupported\n");
						exit(1);
					} else {
						c = c == 'a' ? '\a' :
							c == 'b' ? '\b' :
							c == 't' ? '\t' :
							c == 'n' ? '\n' :
							c == 'v' ? '\v' :
							c == 'f' ? '\f' :
							c == 'r' ? '\r' :
							c == 'e' ? '\e' :
							c;
					}
					backslash = 0;
				} else if (c == '\\') {
					backslash = 1;
					continue;
				}
				if (first) {
					first = 0;
				} else {
					PRINTF(",");
				}
				PRINTF("0x%02x", c);
			}
			PRINTF("\n");
		} else {
			PRINTF("\t.zero %d\n", obj->ty->size);
		}
	}
}

static void emit_text(Obj *prog) {
	for (Obj *obj = prog; obj; obj = obj->next) {
		if (!obj->is_function) {
			continue;
		}
		PRINTF("\t.globl %s\n", obj->name);
		PRINTF("\t.text\n");
		PRINTF("%s:\n", obj->name);
		current_fn = obj;
		// prologue
		PRINTF("\tpush %%rbp\n");
		PRINTF("\tmov %%rsp, %%rbp\n");
		PRINTF("\tsub $%d, %%rsp\n", obj->stack_size);
		// save passed-by-registers args to the stack
		int i = 0;
		for (Obj *var = obj->params; var; var = var->next) {
			PRINTF("\tmov %s, %%rax\n", argreg[i++]);
			switch (var->ty->size) {
			case 8:
				PRINTF("\tmov %%rax, %d(%%rbp)\n", var->offset);
				break;
			case 4:
				PRINTF("\tmov %%eax, %d(%%rbp)\n", var->offset);
				break;
			case 1:
				PRINTF("\tmov %%al, %d(%%rbp)\n", var->offset);
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
		PRINTF(".L.return.%s:\n", obj->name);
		PRINTF("\tmov %%rbp,%%rsp\n");
		PRINTF("\tpop %%rbp\n");
		PRINTF("\tret\n");
	}
}

//void codegen(Obj *prog) {
char *codegen(Obj *prog) {
	assembly_file = open_memstream(&assembly, &assembly_len);

	assign_lvar_offsets(prog);
	emit_data(prog);
	emit_text(prog);

	fflush(assembly_file);
	return assembly;
}