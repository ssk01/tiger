#include "env.h"
E_enventry E_VarEntry(Ty_ty ty) {
	E_enventry e = checked_malloc(sizeof *e);
	e->kind = E_varEntry;
	e->u.var.ty = ty;
	return e;
}
E_enventry E_FunEntry(Ty_tyList formals, Ty_ty result) {
	E_enventry e = checked_malloc(sizeof *e);
	e->kind = E_funEntry;
	e->u.fun.formals = formals;
	e->u.fun.result = result;
	return e;
}
S_table E_base_tenv(void) {
	S_table t = S_empty();
	S_enter(t, S_Symbol(String("int")), Ty_Int());
	S_enter(t, S_Symbol(String("nil")), Ty_Nil());
	S_enter(t, S_Symbol(String("string")), Ty_String());
	return t;
}
S_table E_base_venv(void) {
	S_table t = S_empty();
	Ty_tyList formalTys = NULL;
	formalTys = Ty_TyList(Ty_String(), NULL);
	S_enter(t, S_Symbol(String("print")), E_FunEntry(formalTys, Ty_Void()));
	S_enter(t, S_Symbol(String("getchar")), E_FunEntry(NULL, Ty_String()));
	S_enter(t, S_Symbol(String("ord")), E_FunEntry(formalTys, Ty_Int()));
	formalTys = Ty_TyList(Ty_Int(), NULL);

	S_enter(t, S_Symbol(String("chr")), E_FunEntry(formalTys, Ty_String()));
	return t;
}