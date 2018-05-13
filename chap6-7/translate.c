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
Tr_expList Tr_ExpList(Tr_exp head, Tr_expList tail) {
	Tr_expList e = checked_malloc(sizeof(struct Tr_expList_));
	e->head = head;
	e->tail = tail;
	return e;
}
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




//*********************************************

static Tr_exp Tr_Ex(T_exp ex);

static Tr_exp Tr_Cx(patchList trues, patchList falses, T_stm stm);
static Tr_exp Tr_Nx(T_stm nx);

extern int FRAME_WORD_SIZE;
Tr_exp Tr_simpleVar(Tr_access acc, Tr_level l) {
	T_exp framePtr = T_Temp(F_FP());
	for (; l != acc->level; l = l->parent) {
		//framePtr = T_Mem(T_Binop(T_plus, framePtr,))
		F_access staticLink = F_formals(l->frame)->head;
		framePtr = F_Exp(staticLink, framePtr);
	}
	return Tr_Ex(F_Exp(acc->access, framePtr));
}


Tr_exp Tr_assign(Tr_exp lhs, Tr_exp rhs) {
	return Tr_Nx(T_Move(unEx(lhs), rhs));
}
Tr_exp Tr_ifExp(Tr_exp test, Tr_exp then, Tr_exp elsee) {
	Tr_exp result = NULL;
	Temp_temp r = Temp_newtemp();
	Temp_label t = Temp_newlabel();
	Temp_label f = Temp_newlabel();
	struct Cx condition =  unCx(test);
	do_Patch(condition.trues, t);
	do_Patch(condition.falses, f);
	//to do fast
	if (elsee == NULL) {
		if (then->kind == Tr_cx) {
			Cxinit(then->u.cx, f, f);
			return Tr_Nx(T_Seq(condition.stm,
				T_Seq(T_Label(t),
					T_Seq(then->u.cx.stm, T_Label(f)
					)
				)
			));
		}
		else {
			return Tr_Nx(T_Seq(condition.stm,
							T_Seq(T_Label(t),
								T_Seq(T_Exp(unEx(then)), T_Label(f)
								)
							)
						));
		}
	}
	else {
		Temp_label join = Temp_newlabel();
		T_stm joinJump = T_Jump(T_Name(join), Temp_LabelList(join, NULL));
		return Tr_Nx(T_Eseq(condition.stm,
			T_Seq(T_Label(t),
			T_Seq(T_Exp(unEx(then)), 
			T_Seq(joinJump,
			T_Seq(T_Label(f),
			T_Seq(T_Exp(unEx(elsee)),
			T_Label(join)
					)))))));
		/*if (then->kind == Tr_cx && elsee->kind == Tr_cx) {
			Temp_label z = Temp_newlabel();
			Cxinit(then->u.cx, z, z);
			Cxinit(elsee->u.cx, z, z);
			return Tr_Nx(T_Eseq(condition.stm,
				T_Eseq(T_Label(t),
					T_Eseq(then->u.cx.stm,
						T_Eseq(T_Label(f),
							T_Eseq(elsee->u.cx.stm,
								T_Label(z)
							)
						)
					)
				)));
		}*/
	}
}
Tr_exp Tr_subscriptVar(Tr_exp arr, Tr_exp i) {
	T_exp offset = T_Binop(T_mul, unEx(i), T_Const(FRAME_WORD_SIZE));
	return Tr_Ex(T_Mem(T_Binop(T_plus, unEx(arr), offset)));
}
Tr_exp Tr_fieldVar(Tr_exp exp, int i) {
	T_exp offset = T_Binop(T_mul, T_Const(i), T_Const(FRAME_WORD_SIZE));
	T_exp addr = T_Binop(T_plus, unEx(exp), offset);
	return Tr_Ex(T_Mem(addr));
}

Tr_exp Tr_recordExp(Tr_expList recExpList, int attrNum) {
	int i = 0;
	Temp_temp t;
	T_expList args = T_ExpList(T_Const(attrNum*FRAME_WORD_SIZE), NULL);
	T_stm t_malloc = T_Move(T_Temp(t), ExternCall(String("malloc"), args));
	T_stm moves = T_Move(T_Mem(T_Binop(T_plus, T_Temp(t), T_Const(i*FRAME_WORD_SIZE))), unEx(recExpList->head));
	for (i = attrNum - 2, recExpList = recExpList->tail; i >= 0 && recExpList; i--, recExpList = recExpList->tail) {
		T_stm mov = T_Move(T_Mem(T_Binop(T_plus, T_Temp(t), T_Const(i*FRAME_WORD_SIZE))), unEx(recExpList->head));
		moves = T_Seq(mov, moves);
	}
	return T_Eseq(T_Seq(t_malloc, moves), T_Temp(t));
}
Tr_exp Tr_arithExp(A_oper oper, Tr_exp left, Tr_exp right) {
	T_relOp relop;
	switch (oper)
	{
	case A_eqOp:relop = T_eq; break;
	case A_neqOp:relop = T_ne; break;
	case A_ltOp:relop = T_lt; break;
	case A_leOp:relop = T_le; break;
	case A_geOp:relop = T_ge; break;
	case A_gtOp:relop = T_gt; break;
	default:
		assert(0);
	}
	return Tr_Ex(T_Binop(oper, unEx(left), unEx(right)));

}






void Cxinit(struct Cx c, Temp_label t, Temp_label f) {
	do_Patch(c.trues, t);
	do_Patch(c.falses, f);
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
static struct Cx unCx(Tr_exp e) {
	switch (e->kind)
	{
	case Tr_ex: {
		struct Cx cx;
		cx.stm = T_Cjump(T_ne, e->u.ex, T_Const(0), NULL, NULL);
		cx.trues = PatchList(&cx.stm->u.CJUMP.true, NULL);
		cx.falses = PatchList(&cx.stm->u.CJUMP.false, NULL);
		return cx;
		//return T_Exp(e->u.ex);
	}
	case Tr_nx: {
		assert(0);
	}
	case Tr_cx: {
		//to do
		return e->u.cx;
	}
	default:
		break;
	}
}
static T_stm unNx(Tr_exp e) {
	switch (e->kind)
	{
	case Tr_ex: {
		return T_Exp(e->u.ex);
	}
	case Tr_nx: {
		return e->u.nx;
	}
	case Tr_cx: {
		//to do
		return T_Exp(unEx(e));
	}
	default:
		break;
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
		return T_Eseq(T_Move(T_Temp(r), T_Const(0)), 
			T_Eseq(e->u.cx.stm, 
				T_Eseq(T_Label(f),
					T_Eseq(T_Move(T_Temp(r), T_Const(1)),
						T_Eseq(T_Label(t), T_Temp(r))
					)
				)
			)
		);  
	}
	default:
		assert(0);
		break;
	}
}
