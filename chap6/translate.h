#pragma once
#include "temp.h"
#include "absyn.h"
#include "tree.h"
typedef struct Tr_exp_ *Tr_exp;
typedef struct patchList_ *patchList;
typedef struct Tr_access_ *Tr_access;
typedef struct Tr_level_ *Tr_level;
typedef struct Tr_accessList_ *Tr_accessList;
struct Tr_accessList_ {
	Tr_access head;
	Tr_accessList tail;
};
struct Cx {
	patchList trues; patchList falses; T_stm stm;
};

Tr_accessList Tr_AccessList(Tr_access head, Tr_accessList tail);
Tr_level Tr_outermost(void);

Tr_level Tr_newLevel(Tr_level p, Temp_label n, U_boolList f) ;
struct patchList_ { Temp_label *head; patchList tail; };
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
static T_exp unEx(Tr_exp e);
static T_stm unNx(Tr_exp e);
static struct Cx unCx(Tr_exp e);