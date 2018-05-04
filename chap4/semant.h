#pragma once
#include "types.h"
#include "absyn.h"
typedef  void *Tr_exp;
typedef struct expty expty;
struct expty { Tr_exp exp; Ty_ty ty; };

expty expTy(Tr_exp exp, Ty_ty ty);

expty transVar(S_table venv, S_table tenv, A_var v);

void  transDec(S_table vent, S_table tenv, A_dec d);
expty  transExp(S_table vent, S_table tenv, A_exp a);
Ty_ty transTy(S_table tenv, A_ty d);

