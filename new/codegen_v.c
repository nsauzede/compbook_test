#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "chibicc.h"
#if 0
static void gen(Node *node) {
	if (!node) {
		return;
	}
	bool is_return = false;
	bool is_assign = false;
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
			Node *next = node->body;
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
			gen(node->init);
			printf(";");
			gen(node->cond);
			printf(";");
			gen(node->inc);
			printf("{\n");
			gen(node->then);
			printf("}/*end for*/\n");
			return;
		case ND_NUM:
			printf("%d", node->val);
			return;
		case ND_VAR:
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
		case ND_LT:
			// printf("/*RELINF BOOL*/");
		case ND_EQ:
		case ND_NE:
		case ND_LE:
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

	if (is_return) {
		printf("))");
		return;
	}
	gen(node->rhs);

}
#endif

static void gen_lval(Node *node) {
	if (node->kind != ND_VAR) {
		error("Left side value of assignment is not a variable");
	}
	printf("cv_%s%d", node->var->name, node->var->offset);
}

static void gen_expr(Node *node) {
	if (!node)return;
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
			gen_lval(node);
			goto leave;
		case ND_ASSIGN:
			gen_lval(node->lhs);
			printf(" = ");
			gen_expr(node->rhs);
			printf("\n");
			goto leave;
		case ND_FUNCALL: {
			printf("cf_%s(", node->funcname);
			for (Node *arg = node->args; arg; arg = arg->next) {
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
		printf(") int {\n");
		for (Obj *var = obj->locals; var; var = var->next) {
			if (var->is_param) {
				continue;
			}
			printf("\tmut cv_%s%d := 0//is_param=%d\n", var->name, var->offset, var->is_param);
		}
		gen_stmt(obj->body);
		printf("}\n");
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