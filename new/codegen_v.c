#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "chibicc.h"

static void gen_expr(Node *node);

static void gen_addr(Node *node) {
  switch (node->kind) {
  case ND_VAR:
	printf("cv_%s%d", node->var->name, node->var->offset);
    return;
//   case ND_DEREF:
//     gen_expr(node->lhs);
// 	printf("cv_%s%d", node->var->name, node->var->offset);
//     return;
  }

  error_tok(node->tok, "not an lvalue");
}

static void gen_expr(Node *node) {
	if (!node)return;
	bool is_pointer_arith = false;
	if (node->paren) {
		printf("(");
	}
	switch (node->kind) {
		case ND_NUM:
			printf("%d", node->val);
			goto leave;
		case ND_NEG:
			printf("- ");
			gen_expr(node->lhs);
			goto leave;
		case ND_VAR:
			gen_addr(node);
			goto leave;
		case ND_DEREF:
			printf("*");
			gen_expr(node->lhs);
			return;
		case ND_ADDR:
			printf("&");
			gen_expr(node->lhs);
			return;
		case ND_ASSIGN:
			gen_addr(node->lhs);
			printf(" = ");
			gen_expr(node->rhs);
			printf("\n");
			goto leave;
		case ND_FUNCALL: {
			printf("cf_%s(", node->funcname);
			for (Node *arg = node->args; arg; arg = arg->next) {
				if (arg != node->args) {
					printf(", ");
				}
				gen_expr(arg);
			}
			printf(")");
			goto leave;
		}
	}
	gen_expr(node->lhs);
	switch (node->kind) {
		case ND_EQ:
			printf("==");
			break;
		case ND_NE:
			printf("!=");
			break;
		case ND_LT:
			printf("<");
			break;
		case ND_LE:
			printf("<=");
			break;
		case ND_ADD:
			printf("+");
			if (node->lhs->ty->base) {
				is_pointer_arith = true;
			}
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
		default:
			error("%s: Unknown node type %d\n", __func__, node->kind);
	}
	gen_expr(node->rhs);
	if (is_pointer_arith) {
		printf("/%d", node->lhs->ty->base->size);	// must divide rhs by xx because parser actually added scaling
	}
	leave:
	if (node->paren) {
		printf(")");
	}
}

static void gen_stmt(Node *node) {
	if (!node)return;
	switch (node->kind) {
		case ND_IF:
			printf("if ");
			if (!node->cond->boolean) {
				printf("0!=(");
			}
			gen_expr(node->cond);
			if (!node->cond->boolean) {
				printf(")");
			}
			printf(" {\n");
			gen_stmt(node->then);
			if (node->els) {
				printf("} else {\n");
				gen_stmt(node->els);
			}
			printf("}\n");
			return;
		case ND_FOR:
			printf("for ");
			gen_stmt(node->init);
			printf(";");
			gen_expr(node->cond);
			printf(";");
			gen_expr(node->inc);
			printf("{\n");
			gen_stmt(node->then);
			printf("}\n");
			return;
		case ND_BLOCK:
			for (Node *n = node->body; n; n = n->next) {
				gen_stmt(n);
			}
			return;
		case ND_RETURN:
			printf("\treturn int(");
			gen_expr(node->lhs);
			printf(")\n");
			return;
		case ND_EXPR_STMT:
			gen_expr(node->lhs);
			return;
	}
	error_tok(node->tok, "invalid statement");
}

static void emit_data(Obj *prog) {
	bool is_first = true;
	for (Obj *obj = prog; obj; obj = obj->next) {
		// printf("//obj %s function=%d local=%d\n", obj->name, (int)obj->is_function, (int)obj->is_local);
		if (obj->is_function) {
			continue;
		}
		if (obj->is_local) {
			continue;
		}
		if (is_first) {
			printf("__global (\n");
			is_first = false;
		}
		printf("cv_%s%d int\n", obj->name, obj->offset);
	}
	if (!is_first) {
		printf(")\n");
	}
}

static void emit_text(Obj *prog) {
	for (Obj *obj = prog; obj; obj = obj->next) {
		if (!obj->is_function) {
			continue;
		}
		printf("fn cf_%s(", obj->name);
		for (Obj *param = obj->params; param; param = param->next) {
			if (param != obj->params) {
				printf(", ");
			}
			printf("cv_%s%d int", param->name, param->offset);
		}
		printf(") int {unsafe{\n");
		for (Obj *var = obj->locals; var; var = var->next) {
			if (var->is_param) {
				continue;
			}
			printf("\tmut cv_%s%d := 0//is_param=%d\n", var->name, var->offset, var->is_param);
		}
		gen_stmt(obj->body);
		printf("}}\n");
	}
	printf("\n");
	printf("exit(cf_main())\n");
}

// Assign offsets to local variables.
static void assign_lvar_offsets(Obj *prog) {
  for (Obj *fn = prog; fn; fn = fn->next) {
    if (!fn->is_function) {
		continue;
	}
    int offset = 0;
    for (Obj *var = fn->locals; var; var = var->next) {
		var->offset = offset++;	// fake offset to make unique var names within a func
    }
    for (Obj *param = fn->params; param; param = param->next) {
		param->is_param = true;
    }
  }
}

void codegen_v(Obj *prog) {
	assign_lvar_offsets(prog);
	emit_data(prog);
	emit_text(prog);
}