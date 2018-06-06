#include "env.h"
E_enventry E_VarEntry(Tr_access a, Ty_ty ty) {
	E_enventry final;
	final = checked_malloc(sizeof(*final));
	final->kind = E_varEntry;
	final->u.var.access = a;
	final->u.var.ty = ty;
	return final;
}

E_enventry E_FunEntry(Tr_level level, Temp_label label, Ty_tyList fms, Ty_ty resl, string name) {
	E_enventry final = checked_malloc(sizeof(*final));
	final->kind = E_funEntry;
	final->u.fun.name = name;
	final->u.fun.formals = fms;
	final->u.fun.level = level;
	final->u.fun.label = label;
	final->u.fun.result = resl;
	return final;
}

S_table E_base_tenv(void) {
	S_table t = S_empty();
	S_enter(t, S_Symbol(String("int")), Ty_Int());
	S_enter(t, S_Symbol(String("void")), Ty_Void());
	S_enter(t, S_Symbol(String("nil")), Ty_Nil());
	S_enter(t, S_Symbol(String("string")), Ty_String());
	return t;
}
S_table E_base_venv(void) {
	S_table t = S_empty();
	Ty_tyList formalTys = NULL;
	formalTys = Ty_TyList(Ty_String(), NULL);
	S_enter(t, S_Symbol(String("print")), E_FunEntry(Tr_outermost(), Temp_namedlabel(String("print")), formalTys, Ty_Void(), "print"));
	S_enter(t, S_Symbol(String("printInt")), E_FunEntry(Tr_outermost(), Temp_namedlabel(String("print")), Ty_TyList(Ty_Int(), NULL), Ty_Void(), "printInt"));
	S_enter(t, S_Symbol(String("getchar")), E_FunEntry(Tr_outermost(), Temp_namedlabel(String("getchar")), NULL, Ty_String(), "getchar"));
	S_enter(t, S_Symbol(String("ord")), E_FunEntry(Tr_outermost(), Temp_namedlabel(String("ord")), formalTys, Ty_Int(), "ord"));
	formalTys = Ty_TyList(Ty_Int(), NULL);

	S_enter(t, S_Symbol(String("chr")), E_FunEntry(Tr_outermost(), Temp_namedlabel(String("chr")), formalTys, Ty_String(), "chr"));
	return t;
}