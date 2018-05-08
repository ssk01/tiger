#include "escape.h" 
#include "util.h"
typedef struct Escapeentry_* Escapeentry;
struct Escapeentry_ {
	int depth;
	bool *escape;
};
Escapeentry EscapeEntry(int depth, bool *escape) {
	*escape = FALSE;
	Escapeentry e = (Escapeentry)checked_malloc(sizeof(struct Escapeentry_));
	e->depth = depth;
	e->escape = escape;
	return e;
};
void Esc_findEscape(A_exp exp) {

}
static void tranverseExp(S_table env, int depth, A_exp e) {

}
static void tranverseDec(S_table env, int depth, A_dec d) {
	switch (d->kind)
	{
	case A_varDec: {
		//d->u.var.escape = FALSE;
		S_enter(env, d->u.var.var, EscapeEntry(depth, &(d->u.var.escape)));
		break;
	}
	case A_functionDec: {
		A_fundecList list;
		for (list = d->u.function; list; list = list->tail) {
			A_fundec f = list->head;
			
			S_beginScope(env);
			{
				A_fieldList l;
				for (l = f->params; l; l = l->tail) {
					S_enter(env, l->head->name, EscapeEntry(depth, &(l->head->escape)));
				}
				tranverseExp(env, depth+1, list->head->body);
			}
			S_endScope(env);
		}
		break;
	}
	case A_typeDec: {
		
		break;
	}
	default:
		break;
	}
}
static void tranverseVar(S_table env, int depth, A_var v) {
	switch (v->kind)
	{
	case A_simpleVar: {
		Escapeentry e = S_look(env, v->u.simple);
		if (e->depth > depth) {
			*(e->escape) = TRUE;
		}
		break;
	}
	case A_subscriptVar: {
		transVar(env, env, v->u.subscript.var);
		break;
	}
	case A_fieldVar: {
		transVar(env, depth, v->u.field.var);
		break;
	}
	default: {
		assert(0);
	}
		
	}
}