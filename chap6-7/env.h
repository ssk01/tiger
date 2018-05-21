#pragma once
#include "types.h"
//#include "table.h"
#include "symbol.h"
#include "translate.h"
typedef struct E_enventry_ *E_enventry;
struct E_enventry_ {
	enum {E_varEntry, E_funEntry} kind;
	union {
		struct { Tr_access access; Ty_ty ty; } var;
		struct {
			string *name;
			Tr_level level;
			Temp_label label;
			Ty_tyList formals; Ty_ty result; } fun;
	} u;
};
E_enventry E_VarEntry(Tr_access a, Ty_ty ty);
E_enventry E_FunEntry(Tr_level level, Temp_label label, Ty_tyList fms, Ty_ty resl, string name);
S_table E_base_tenv(void);
S_table E_base_venv(void);


