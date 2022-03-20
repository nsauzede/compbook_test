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
static char *argreg8[] = {"%dil", "%sil", "%dl", "%cl", "%r8b", "%r9b"};
static char *argreg16[] = {"%di", "%si", "%dx", "%cx", "%r8w", "%r9w"};
static char *argreg32[] = {"%edi", "%esi", "%edx", "%ecx", "%r8d", "%r9d"};
static char *argreg64[] = {"%rdi", "%rsi", "%rdx", "%rcx", "%r8", "%r9"};
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

int align_to(int n, int align) {
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
	case ND_COMMA:
		gen_expr(node->lhs);
		gen_addr(node->rhs);
		return;
	case ND_MEMBER:
		gen_addr(node->lhs);
		PRINTF("\tadd $%d, %%rax\n", node->member->offset);
		return;
  }

  error_tok(node->tok, "not an lvalue");
}

// Load a value from where %rax is pointing to.
static void load(Node *node) {
if (node->ty->kind == TY_ARRAY || node->ty->kind == TY_STRUCT || node->ty->kind == TY_UNION) {
    // If it is an array, do not attempt to load a value to the
    // register because in general we can't load an entire array to a
    // register. As a result, the result of an evaluation of an array
    // becomes not the array itself but the address of the array.
    // This is where "array is automatically converted to a pointer to
    // the first element of the array in C" occurs.
    return;
  }
	switch (node->ty->size) {
	case 1:
		PRINTF("\tmovsbq (%%rax), %%rax\n");
		break;
	case 2:
		PRINTF("\tmovswq (%%rax), %%rax\n");
		break;
	case 4:
		PRINTF("\tmovsxd (%%rax), %%rax\n");
		break;
	case 8:
		PRINTF("\tmov (%%rax), %%rax\n");
		break;
	default:
		error_tok(node->tok, "unsupported load size %d", node->ty->size);
	}
}

// Store %rax to an address that the stack top is pointing to.
static void store(Node *node) {
	pop("%rdi");
	if (node->ty->kind == TY_STRUCT || node->ty->kind == TY_UNION) {
		for (int i = 0; i < node->ty->size; i++) {
			PRINTF("\tmov %d(%%rax), %%r8b\n", i);
			PRINTF("\tmov %%r8b, %d(%%rdi)\n", i);
		}
		return;
	}
	switch (node->ty->size) {
	case 1:
		PRINTF("\tmov %%al, (%%rdi)\n");
		break;
	case 2:
		PRINTF("\tmov %%ax, (%%rdi)\n");
		break;
	case 4:
		PRINTF("\tmov %%eax, (%%rdi)\n");
		break;
	case 8:
		PRINTF("\tmov %%rax, (%%rdi)\n");
		break;
	default:
		error_tok(node->tok, "unsupported store size %d", node->ty->size);
	}
}

static void gen_expr(Node *node) {
	PRINTF("  .loc 1 %d  /* %s */\n", node->tok->line_no, __func__);
	switch (node->kind) {
		case ND_NUM:
			PRINTF("\tmov $%ld, %%rax\n", node->val);
			return;
		case ND_NEG:
			gen_expr(node->lhs);
			PRINTF("\tneg %%rax\n");
			return;
		case ND_VAR:
		case ND_MEMBER:
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
		case ND_COMMA:
			gen_expr(node->lhs);
			gen_expr(node->rhs);
			return;
		case ND_FUNCALL: {
			int nargs = 0;
			for (Node *arg = node->args; arg; arg = arg->next) {
				gen_expr(arg);
				push();
				nargs++;
			}
			for (int i = nargs - 1; i >= 0; i--) {
				pop(argreg64[i]);
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
	char *ax, *di;
	if (node->lhs->ty->kind == TY_LONG || node->lhs->ty->base) {
		ax = "%rax";
		di = "%rdi";
	} else {
		ax = "%eax";
		di = "%edi";
	}
	switch (node->kind) {
		case ND_ADD:
			PRINTF("\tadd %s, %s\n", di, ax);
			return;
		case ND_SUB:
			PRINTF("\tsub %s, %s\n", di, ax);
			return;
		case ND_MUL:
			PRINTF("\timul %s, %s\n", di, ax);
			return;
		case ND_DIV:
			if (node->lhs->ty->size == 8) {
				PRINTF("\tcqo\n");
			} else {
				PRINTF("\tcdq\n");
			}
			PRINTF("\tidiv %s\n", di);
			return;
		case ND_EQ:
		case ND_NE:
		case ND_LT:
		case ND_LE:
			PRINTF("\tcmp %s, %s\n", di, ax);
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
	PRINTF("  .loc 1 %d  /* %s */ \n", node->tok->line_no, __func__);
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
			offset = align_to(offset, var->ty->align);
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
			int backslash = 0, hex = 0, oct = 0;
			int num = 0, count = 0;
			int first = 1;
			for (int i = 0; i < obj->ty->size; i++) {
				unsigned char c = ((unsigned char *)obj->init_data)[i];
				if (backslash) {
					if (hex) {
						if (isxdigit(c)) {
							num <<= 4;
							if (c <= '9') {
								c -= '0';
							} else {
								c |= ' ';
								c -= 'a';
							}
							num += c;
							continue;
						} else {
							if (first) {
								first = 0;
							} else {
								PRINTF(",");
							}
							PRINTF("0x%02x", num);
						}
					} else if (oct) {
						if (c >= '0' && c <= '7' && count < 3) {
							num <<= 3;
							num += c - '0';
							count++;
							continue;
						} else {
							if (first) {
								first = 0;
							} else {
								PRINTF(",");
							}
							PRINTF("0x%02x", num);
						}
					} else if (c == 'x') {
						hex = 1;
						continue;
					} else if (c >= '0' && c <= '7') {
						oct = 1;
						num <<= 3;
						num += c - '0';
						count++;
						continue;
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
					hex = 0;
					oct = 0;
					count = 0;
					num = 0;
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

static void store_gp(int r, int offset, int sz) {
	switch (sz) {
	case 1:
		PRINTF("\tmov %s, %d(%%rbp)\n", argreg8[r], offset);
		break;
	case 2:
		PRINTF("\tmov %s, %d(%%rbp)\n", argreg16[r], offset);
		break;
	case 4:
		PRINTF("\tmov %s, %d(%%rbp)\n", argreg32[r], offset);
		break;
	case 8:
		PRINTF("\tmov %s, %d(%%rbp)\n", argreg64[r], offset);
		break;
	default:
		unreachable();
	}
}

static void emit_text(Obj *prog) {
	for (Obj *obj = prog; obj; obj = obj->next) {
		if (!obj->is_function || !obj->is_definition) {
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
			store_gp(i++, var->offset, var->ty->size);
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

char *codegen(Obj *prog, char *input_file) {
	assembly_file = open_memstream(&assembly, &assembly_len);
	PRINTF(".file 1 \"%s\"\n", input_file);

	assign_lvar_offsets(prog);
	emit_data(prog);
	emit_text(prog);

	fflush(assembly_file);
	return assembly;
}
