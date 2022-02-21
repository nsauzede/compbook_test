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
	printf("/*gen_addr VAR*/");
#if 0
	printf("/*gen_addr node=%d:%s",
		node->kind,
		node->ty->kind == TY_ARRAY?"arr":
		node->ty->kind == TY_FUNC?"fun":
		node->ty->kind == TY_INT?"int":
		node->ty->kind == TY_PTR?"ptr":
		"???"
	);
	if (node->lhs)
	printf(" lhs=%d:%s",
		node->lhs->kind,
		node->lhs->ty->kind == TY_ARRAY?"arr":
		node->lhs->ty->kind == TY_FUNC?"fun":
		node->lhs->ty->kind == TY_INT?"int":
		node->lhs->ty->kind == TY_PTR?"ptr":
		"???"
	);
	if (node->rhs)
	printf(" rhs=%d:%s",
		node->rhs->kind,
		node->rhs->ty->kind == TY_ARRAY?"arr":
		node->rhs->ty->kind == TY_FUNC?"fun":
		node->rhs->ty->kind == TY_INT?"int":
		node->rhs->ty->kind == TY_PTR?"ptr":
		"???"
	);
	printf("*/");
#endif
	if (node->ty->kind == TY_ARRAY) {
		printf("&");
	}
	printf("cv_%s%d", node->var->name, node->var->offset);
	if (node->ty->kind == TY_ARRAY) {
		printf("[0]");
	}
    return;
  case ND_DEREF:
	printf("/*gen_addr DEREF*/");
#if 0
	printf("/*gen_addr node=%d:%s",
		node->kind,
		node->ty->kind == TY_ARRAY?"arr":
		node->ty->kind == TY_FUNC?"fun":
		node->ty->kind == TY_INT?"int":
		node->ty->kind == TY_PTR?"ptr":
		"???"
	);
	if (node->lhs)
	printf(" lhs=%d:%s",
		node->lhs->kind,
		node->lhs->ty->kind == TY_ARRAY?"arr":
		node->lhs->ty->kind == TY_FUNC?"fun":
		node->lhs->ty->kind == TY_INT?"int":
		node->lhs->ty->kind == TY_PTR?"ptr":
		"???"
	);
	if (node->rhs)
	printf(" rhs=%d:%s",
		node->rhs->kind,
		node->rhs->ty->kind == TY_ARRAY?"arr":
		node->rhs->ty->kind == TY_FUNC?"fun":
		node->rhs->ty->kind == TY_INT?"int":
		node->rhs->ty->kind == TY_PTR?"ptr":
		"???"
	);
	printf("*/");
#endif
	if (node->var || (node->lhs && node->lhs->ty->kind == TY_PTR)) {
		printf("*");
	} else if (/*node->lhs->kind == ND_VAR &&*/ node->lhs->ty->kind == TY_ARRAY) {
	 	// printf("&");
	} else {
	}
    gen_expr(node->lhs);
	if (node->ty->kind == TY_ARRAY) {
		printf("/*1*/[0]");
	}
    return;
  }

  error_tok(node->tok, "not an lvalue");
}

static void gen_expr(Node *node) {
	if (!node)return;
	bool is_pointer_arith = false;
	if (node->paren) {
		printf("/*paren*/(");
	}
	switch (node->kind) {
		case ND_NUM:
			printf("%ld", node->val);
			goto leave;
		case ND_NEG:
			printf("- ");
			gen_expr(node->lhs);
			goto leave;
		case ND_VAR:
			printf("/*gen_expr VAR*/");
			gen_addr(node);
			goto leave;
		case ND_DEREF:
			printf("/*gen_expr DEREF*/");
#if 0
			printf("/*gen_expr node=%d:%s",
				node->kind,
				node->ty->kind == TY_ARRAY?"arr":
				node->ty->kind == TY_FUNC?"fun":
				node->ty->kind == TY_INT?"int":
				node->ty->kind == TY_PTR?"ptr":
				"???"
			);
			if (node->lhs)
			printf(" lhs=%d:%s",
				node->lhs->kind,
				node->lhs->ty->kind == TY_ARRAY?"arr":
				node->lhs->ty->kind == TY_FUNC?"fun":
				node->lhs->ty->kind == TY_INT?"int":
				node->lhs->ty->kind == TY_PTR?"ptr":
				"???"
			);
			if (node->rhs)
			printf(" rhs=%d:%s",
				node->rhs->kind,
				node->rhs->ty->kind == TY_ARRAY?"arr":
				node->rhs->ty->kind == TY_FUNC?"fun":
				node->rhs->ty->kind == TY_INT?"int":
				node->rhs->ty->kind == TY_PTR?"ptr":
				"???"
			);
			printf("*/");
#endif
			printf("*");
			if (node->lhs->ty->kind == TY_ARRAY) {
				// printf("&");
			}
			gen_expr(node->lhs);
			if (node->lhs->ty->kind == TY_ARRAY) {
				// printf("/*2*/[0]");
			}
			return;
		case ND_ADDR:
			printf("/*gen_expr ADDR*/");
			printf("&");
			gen_expr(node->lhs);
			if (node->lhs->ty->kind == TY_ARRAY) {
				printf("/*3*/[0]");
			}
			return;
		case ND_ASSIGN: {
			printf("/*gen_expr ASSIGN*/");
			static int count = 0;
			bool use_temp = false;
#if 1
			printf("/*node=%d:%s lhs=%d:%s rhs=%d:%s*/",
				node->kind,
				node->ty->kind == TY_ARRAY?"arr":
				node->ty->kind == TY_FUNC?"fun":
				node->ty->kind == TY_INT?"int":
				node->ty->kind == TY_PTR?"ptr":
				"???",
				node->lhs->kind,
				node->lhs->ty->kind == TY_ARRAY?"arr":
				node->lhs->ty->kind == TY_FUNC?"fun":
				node->lhs->ty->kind == TY_INT?"int":
				node->lhs->ty->kind == TY_PTR?"ptr":
				"???",
				node->rhs->kind,
				node->rhs->ty->kind == TY_ARRAY?"arr":
				node->rhs->ty->kind == TY_FUNC?"fun":
				node->rhs->ty->kind == TY_INT?"int":
				node->rhs->ty->kind == TY_PTR?"ptr":
				"???"
			);
			if (node->lhs->lhs) {
			printf("/*lhs=%d:%s*/",
				node->lhs->lhs->kind,
				node->lhs->lhs->ty->kind == TY_ARRAY?"arr":
				node->lhs->lhs->ty->kind == TY_FUNC?"fun":
				node->lhs->lhs->ty->kind == TY_INT?"int":
				node->lhs->lhs->ty->kind == TY_PTR?"ptr":
				"???"
			);
			}
#endif
			// if (!node->lhs->var /*&& node->lhs->kind != ND_DEREF*/) {
			// if (!node->lhs->var && (node->lhs->lhs && node->lhs->lhs->ty->kind == TY_ARRAY)) {
			if (!node->lhs->var && (node->lhs->lhs && node->lhs->lhs->kind != ND_VAR)) {
				use_temp = true;
				// printf("[0");
			}
			if (use_temp) {
				printf("{cvtmp%d := ", count);
			}
			gen_addr(node->lhs);
			if (use_temp) {
				// printf("\t//temporary because lvalue not a var\n*cvtmp%d", count++);
				// printf("]");
				printf("{*cvtmp%d", count);
			}
			if (!use_temp && node->lhs->var && !node->lhs->var->already_assigned) {
				node->lhs->var->already_assigned = true;
				printf(" := ");
			} else {
				printf(" = ");
			}
			if (node->lhs->ty->kind == TY_PTR && node->rhs->ty->kind == TY_ARRAY) {
				printf("&/*gen_expr*/");
			}
			gen_expr(node->rhs);
			if (node->lhs->ty->kind == TY_PTR && node->rhs->ty->kind == TY_ARRAY) {
				Type *ty = node->rhs->ty;
				do {
					printf("/*4*/[0]");
					ty = ty->base;
				} while (ty && ty->kind == TY_ARRAY);
			}
			if (use_temp) {
				printf("}}");
			}
			printf("	//var=%p\n", node->lhs->var);
			goto leave;
		}
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
		case ND_ADD:
			printf("/*gen_expr ADD*/");
			if (node->lhs->ty->kind == TY_ARRAY) {
				// printf("/*add*/&");
			}
			break;
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
			if (node->lhs->ty->kind == TY_ARRAY) {
				// printf("/*add*/[0]");
			}
			printf("+");
			break;
		case ND_SUB:
			printf("-");
			break;
		case ND_MUL:
			if (node->pointer_arith) {
				goto leave;
			}
			printf("*");
			break;
		case ND_DIV:
			if (node->pointer_arith) {
				goto leave;
			}
			printf("/");
			break;
		default:
			error("%s: Unknown node type %d\n", __func__, node->kind);
	}
	gen_expr(node->rhs);
	leave:
	if (node->paren) {
		printf("/*paren*/)");
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
		// printf("/*name=%s func=%d loc=%d par=%d arr=%d*/\n", obj->name, obj->is_function, obj->is_local, obj->is_param, obj->ty->kind == TY_ARRAY);
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
			// printf("\tmut cv_%s%d := 0//is_param=%d\n", var->name, var->offset, var->is_param);
			printf("/*name=%s func=%d loc=%d par=%d arr=%d size=%d*/\n", var->name, var->is_function, var->is_local, var->is_param, var->ty->kind == TY_ARRAY, var->ty->size);
			if (var->ty->kind == TY_ARRAY) {
				// if (var->ty->base->kind == TY_ARRAY) {
				// 	fprintf(stderr, "array of array ?\n");
				// 	exit(1);
				// }
				printf("mut cv_%s%d := ", var->name, var->offset);
				Type *ty = var->ty;
				do {
					printf("[%d]", ty->array_len);
					ty = ty->base;
				} while (ty && ty->kind == TY_ARRAY);
				printf("int{}\n");
			}
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
		// printf("/*local var %s ofs %d is_array=%d*/\n", var->name, var->offset, var->ty->kind == TY_ARRAY);
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