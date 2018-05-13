#pragma once
#include "types.h"
#include "absyn.h"
#include"translate.h"
//typedef  void *Tr_exp;
typedef struct expty expty;
struct expty { Tr_exp exp; Ty_ty ty; };
void SEM_transProg(A_exp exp);
expty expTy(Tr_exp exp, Ty_ty ty);

expty transVar(Tr_level level, S_table venv, S_table tenv, A_var v);

Tr_exp  transDec(Tr_level level, S_table vent, S_table tenv, A_dec d);
expty  transExp(Tr_level level, S_table vent, S_table tenv, A_exp a);
Ty_ty transTy(S_table tenv, A_ty d);

void Ty_tyKind(Ty_ty e);