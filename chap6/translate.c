#include "translate.h"
#include "temp.h"
#include "tree.h"
#include "frame.h"
#include "env.h"
struct Tr_level_ {
	Tr_level parent;
	Temp_label name;
	F_frame frame;
	Tr_accessList formals;
};

struct Tr_access_ {
	Tr_level level;
	F_access access;
};
Tr_access newTr_access(Tr_level level, F_access access) {
	Tr_access a = checked_malloc(sizeof(*a));
	a->access = access;
	a->level = level;
	return a;
}
Tr_accessList Tr_formals(Tr_level l) {
	return l->formals;
	//F_accessList f = F_formals(l->frame);
	//Tr_accessList res = NULL;
	//for (; f; f = f->tail) {
	//	res = Tr_AccessList(newTr_access(l, f->head), res);
	//}
	//return res;
}

Tr_access Tr_allocLocal(Tr_level l, bool escape) {
	return newTr_access(l, F_allocLocal(l->frame, escape));
}

Tr_accessList makeFormalAccessList(Tr_level l) {
	Tr_accessList trl = NULL;
	Tr_access tr = NULL;
	F_accessList acl = F_formals(l->frame)->tail;
	for (; acl; acl = acl->tail) {
		tr = newTr_access(l, acl->head);
		trl = Tr_AccessList(tr, trl);
	}
	return trl;
}
Tr_level Tr_newLevel(Tr_level p, Temp_label n, U_boolList f)
{
	Tr_level l = checked_malloc(sizeof(*l));
	l->parent = p;
	l->name = n;
	l->frame = F_newFrame(n, U_BoolList(TRUE, f));
	l->formals = makeFormalAccessList(l);
	return l;
}
static Tr_level outer = NULL;
Tr_level Tr_outermost(void) {
	if (outer == NULL) {
		outer = Tr_newLevel(NULL, Temp_newlabel(), NULL);
	}
	return outer;
}


Tr_accessList Tr_AccessList(Tr_access head, Tr_accessList tail) {
	Tr_accessList al = checked_malloc(sizeof(*al));
	al->head = head;
	al->tail = tail;
	return al;
}



static patchList PatchList(Temp_label *head, patchList tail) {
	patchList pl = (patchList)checked_malloc(sizeof(struct patchList_));
	pl->head = head;
	pl->tail = tail;
	return pl;
}

static Tr_exp Tr_Ex(T_exp ex) {
	Tr_exp e = checked_malloc(sizeof(*e));
	e->kind = Tr_ex;
	e->u.ex = ex;
	return e;
}
static Tr_exp Tr_Nx(T_stm nx) {
	Tr_exp e = checked_malloc(sizeof(*e));
	e->kind = Tr_nx;
	e->u.nx = nx;
	return e;
}
static Tr_exp Tr_Cx(patchList trues, patchList falses, T_stm stm) {
	Tr_exp e = checked_malloc(sizeof(*e));
	e->kind = Tr_cx;
	e->u.cx.falses = falses;
	e->u.cx.trues = trues;
	e->u.cx.stm = stm;
	return e;
}


void do_Patch(patchList tList, Temp_label label) {
	for (; tList; tList = tList->tail) {
		*(tList->head) = label;
	}
}
static T_exp unEx(Tr_exp e) {
	switch (e->kind)
	{
	case Tr_ex: {
		return e->u.ex;
	}
	case Tr_nx: {
		return T_Eseq(e->u.nx, T_Const(0));
	}
	case Tr_cx: {
		Temp_temp r = Temp_newtemp();
		Temp_label t = Temp_newlabel();
		Temp_label f = Temp_newlabel();
		do_Patch(e->u.cx.falses, f);
		do_Patch(e->u.cx.trues, t);
		return T_Eseq(T_Move(T_Temp(r), T_Const(0)), NULL
			/*T_Eseq(e->u.cx.stm, 
				T_Eseq(T_Label(f),
					T_Eseq(T_Move(T_Temp(r), T_Const(1)),
						T_Eseq(T_Label(t), T_Temp(r))
					)
				)
			)*/
		);  
	}
	default:
		assert(0);
		break;
	}
}

