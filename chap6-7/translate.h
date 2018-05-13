#pragma once
#include "temp.h"
#include "absyn.h"
#include "tree.h"
#include "frame.h"
typedef struct Tr_exp_ *Tr_exp;
typedef struct Tr_expList_ *Tr_expList;
typedef struct patchList_ *patchList;
typedef struct Tr_access_ *Tr_access;
typedef struct Tr_level_ *Tr_level;
typedef struct Tr_accessList_ *Tr_accessList;
struct Tr_expList_ {
	Tr_exp head;
	Tr_expList tail;
};
Tr_expList Tr_ExpList(Tr_exp head,Tr_expList tail);
struct Tr_accessList_ {
	Tr_access head;
	Tr_accessList tail;
};
struct Cx {
	patchList trues; patchList falses; T_stm stm;
};
Tr_access newTr_access(Tr_level level, F_access access);
Tr_accessList Tr_AccessList(Tr_access head, Tr_accessList tail);
Tr_level Tr_outermost(void);
Tr_access Tr_allocLocal(Tr_level l, bool escape);
Tr_level Tr_newLevel(Tr_level p, Temp_label n, U_boolList f) ;
struct patchList_ { Temp_label *head; patchList tail; };

Tr_accessList Tr_formals(Tr_level l);

typedef struct Tr_exp_ *Tr_exp;
struct Tr_exp_ {
	enum {
		Tr_ex, Tr_nx, Tr_cx
	} kind;
	union
	{
		T_exp ex;
		T_stm nx;
		struct Cx cx;
	} u;
};

Tr_exp Tr_simpleVar(Tr_access acc, Tr_level l);
Tr_exp Tr_subscriptVar(Tr_exp arr, Tr_exp i);
Tr_exp Tr_ifExp(Tr_exp test, Tr_exp then, Tr_exp elsee);
Tr_exp Tr_fieldVar(Tr_exp exp, int i);
Tr_exp Tr_assign(Tr_exp lhs, Tr_exp rhs);
Tr_exp Tr_recordExp(Tr_expList recExpList, int attrNum);
Tr_exp Tr_arithExp(A_oper oper, Tr_exp left, Tr_exp right);
void do_Patch(patchList tList, Temp_label label);
static T_exp unEx(Tr_exp e);
static T_stm unNx(Tr_exp e);
static struct Cx unCx(Tr_exp e);
void Cxinit(struct Cx c, Temp_label t, Temp_label f);