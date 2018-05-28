#include "translate.h"
#include "temp.h"
#include "tree.h"
#include "frame.h"
#include "env.h"
#include "printtree.h"
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
int trLevelParent(Tr_level l) {
	if (l->parent == NULL) {
		return 0;
	}
	return 1;
}
static patchList PatchList(Temp_label *head, patchList tail);
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
	printf("Tr_allocLocal\n");
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
	l->frame = F_newFrame(n, U_BoolList(FALSE, f));
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
	//?parent
	for (; l != acc->level; l = l->parent) {
		//framePtr = T_Mem(T_Binop(T_plus, framePtr,))
		F_access staticLink = F_formals(l->frame)->head;
		framePtr = F_Exp(staticLink, framePtr);
	}
	return Tr_Ex(F_Exp(acc->access, framePtr));
}
Tr_exp Tr_whileExp(Tr_exp test, Tr_exp body, Tr_exp done) {
	Temp_label doneLabel = unEx(done)->u.NAME;
	Temp_label testLabel = Temp_newlabel();
	Temp_label bodyLabel = Temp_newlabel();
	T_stm test_stm = T_Cjump(T_eq, unEx(test), T_Const(0), doneLabel, bodyLabel);
	return Tr_Nx(
	T_Seq(T_Label(testLabel),
	T_Seq(test_stm,
	T_Seq(T_Label(bodyLabel),
	T_Seq(unNx(body),
	T_Seq(T_Jump(T_Name(testLabel), Temp_LabelList(testLabel, NULL)),
	 T_Label(doneLabel)))))));
}

Tr_exp Tr_assign(Tr_exp lhs, Tr_exp rhs) {
	return Tr_Nx(T_Move(unEx(lhs), unEx(rhs)));
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
		//return Tr_Nx(condition.stm);
		return Tr_Nx(T_Seq(condition.stm,
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
Tr_exp Tr_breakInit() {
	return Tr_Ex(T_Name(Temp_newlabel()));
}

Tr_exp Tr_breakExp(Tr_exp breakk) {
	assert(breakk != NULL);
	return Tr_Nx(T_Jump(unEx(breakk), Temp_LabelList(unEx(breakk)->u.NAME, NULL)));
}
F_frag F_ProcFrag(T_stm body, string name, F_frame frame) {
	F_frag f = checked_malloc(sizeof(*f));
	f->kind = F_procFrag;
	f->u.proc.name = name;
	f->u.proc.body = body;
	f->u.proc.frame = frame;
	return f;
}
F_frag F_StringFrag(Temp_label lable, string str) {
	F_frag f = checked_malloc(sizeof(*f));
	f->kind = F_stringFrag;
	f->u.stringg.label = lable;
	f->u.stringg.str = str;
	return f;
}
F_fragList	F_FragList(F_frag head, F_fragList tail) {
	F_fragList f = checked_malloc(sizeof(struct F_fragList_));
	f->head = head;
	f->tail = tail;
	return f;
}


F_fragList fragList = NULL;
void Tr_procEntryExit(Tr_level level, string name, Tr_exp body) {
	F_frag proc_flag = F_ProcFrag(unNx(body), name, level->frame);
	fragList = F_FragList(proc_flag, fragList);
}

F_fragList stringFragList = NULL;
Tr_exp Tr_stringExp(string s) {
	Temp_label label = Temp_newlabel();
	F_frag string_frag = F_StringFrag(label, s);
	stringFragList = F_FragList(string_frag, stringFragList);
	return Tr_Ex(T_Name(label));
}

static Temp_temp nil_temp = NULL;
Tr_exp Tr_noExp() {
	return Tr_Ex(T_Const(0));
}
Tr_exp Tr_nilExp() {
	if (!nil_temp) {
		nil_temp = Temp_newtemp();
		T_stm t_malloc = T_Move(T_Temp(nil_temp), ExternCall(String("malloc"), T_ExpList(T_Const(0), NULL)));
		return Tr_Ex(T_Eseq(t_malloc, T_Temp(nil_temp)));
	}
	return Tr_Ex(T_Temp(nil_temp));
}
Tr_exp Tr_intExp(int i) {
	return Tr_Ex(T_Const(i));
}

Tr_exp Tr_letExp(Tr_expList explist) {
	return Tr_seqExp(explist);
}



Tr_exp Tr_callExp(Temp_label fun, Tr_level def_level, Tr_level call_level, Tr_expList args) {
	//ARGS SEQ IS FU DE.
	T_exp framePtr = T_Temp(F_FP());
	while (call_level && call_level != def_level->parent) {
		F_access staticLink = F_formals(call_level->frame)->head;
		framePtr = F_Exp(staticLink, framePtr);
		call_level = call_level->parent;
	}
	T_expList targs = T_ExpList(framePtr, NULL);
	T_expList realargs = NULL;
	for (; args; args = args->tail) {
		T_exp arg = unEx(args->head);
		realargs = T_ExpList(arg, realargs);
	}
	targs->tail = realargs;
	return Tr_Ex(T_Call(T_Name(fun), targs));
		//framePtr = T_Mem(T_Binop(T_plus, framePtr,))
}

Tr_exp Tr_arrayExp(Tr_exp size, Tr_exp init) {
	T_expList args = T_ExpList(unEx(size), T_ExpList(unEx(init), NULL));
	return Tr_Ex(ExternCall(String("initArray"), args));
}

Tr_exp Tr_recordExp(Tr_expList recExpList, int attrNum) {
	int i = attrNum-1;
	Temp_temp t = Temp_newtemp();
	T_expList args = T_ExpList(T_Const(attrNum*FRAME_WORD_SIZE), NULL);
	T_stm t_malloc = T_Move(T_Temp(t), ExternCall(String("malloc"), args));
	T_stm moves = T_Move(T_Mem(T_Binop(T_plus, T_Temp(t), T_Const(i*FRAME_WORD_SIZE))), unEx(recExpList->head));
	for (i = attrNum - 2, recExpList = recExpList->tail; i >= 0 && recExpList; i--, recExpList = recExpList->tail) {
		T_stm mov = T_Move(T_Mem(T_Binop(T_plus, T_Temp(t), T_Const(i*FRAME_WORD_SIZE))), unEx(recExpList->head));
		moves = T_Seq(mov, moves);
	}
	return Tr_Ex(T_Eseq(T_Seq(t_malloc, moves), T_Temp(t)));
}
// it's reversed
Tr_exp Tr_seqExp(Tr_expList seqList) {
	T_exp seq = unEx(seqList->head);
	seqList = seqList->tail;
	while (seqList) {
		seq = T_Eseq(unNx(seqList->head), seq);
		seqList = seqList->tail;
	}
	return Tr_Ex(seq);
}


Tr_exp Tr_stringCmpExp(A_oper op, Tr_exp left_exp, Tr_exp right_exp) //default signed int
{
	T_expList args = T_ExpList(unEx(left_exp),
		T_ExpList(unEx(right_exp), NULL)
	);
	T_exp list = ExternCall(String("stringEqual"), args);
	switch (op) {
	case A_eqOp: {
		return Tr_Ex(list);
	}
	case A_neqOp: {
		return (list->kind == T_CONST && list->u.CONST == 1) ? Tr_Ex(T_Const(0)) : Tr_Ex(T_Const(1));
	}
	default:
		assert(0);
	}

}

Tr_exp Tr_comOpExp(A_oper op, Tr_exp left_exp, Tr_exp right_exp) //default signed int
{
	T_relOp t_relop;
	switch (op)
	{
	case A_eqOp: t_relop = T_eq; break;
	case A_neqOp: t_relop = T_ne; break;
	case A_ltOp: t_relop = T_lt; break;
	case A_leOp: t_relop = T_le; break;
	case A_gtOp: t_relop = T_gt; break;
	case A_geOp: t_relop = T_ge; break;
	default: assert(0); /* should never happen*/
	}
	T_stm stm = T_Cjump(t_relop, unEx(left_exp), unEx(right_exp), NULL, NULL);
	patchList trues = PatchList(&(stm->u.CJUMP.true), NULL);
	patchList falses = PatchList(&(stm->u.CJUMP.false), NULL);
	return Tr_Cx(trues, falses, stm);
}


Tr_exp Tr_binOp(A_oper op, Tr_exp left_exp, Tr_exp right_exp)
{
	T_binOp t_binop;
	switch (op)
	{
	case A_plusOp: t_binop = T_plus; break;
	case A_minusOp: t_binop = T_minus; break;
	case A_timesOp: t_binop = T_mul; break;
	case A_divideOp: t_binop = T_div; break;
	}
	T_exp binop = T_Binop(t_binop, unEx(left_exp), unEx(right_exp));
	return Tr_Ex(binop);
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

extern F_fragList fragList;
extern F_fragList stringFragList;
void pr_tr(FILE *out, Tr_exp e, int d) {
	if (e->kind == Tr_ex) {
		pr_tree_exp(out, e->u.ex, d);
	}
	if (e->kind == Tr_nx) {
		pr_stm(out, e->u.nx, d);
	}
	if (e->kind == Tr_cx) {
		pr_stm(out, e->u.cx.stm, d);
	}
}
F_fragList Tr_getResult() {
	F_fragList prev = stringFragList;
	if (!prev) {
		return fragList;
	}
	while(prev->tail) {
		prev = prev->tail;
	}
	prev->tail = fragList;
	return stringFragList;
}
void printFrag() {
	F_fragList l = fragList;
	printf("frame frag \n");
	for (; l; l = l->tail) {
		printf("  frame:   %s %s\n", l->head->u.proc.name, S_name(F_name(l->head->u.proc.frame)));
		//void pr_tr(FILE *out, Tr_exp e, int d);
		pr_tr(stdout, Tr_Nx( l->head->u.proc.body), 4);
		printf("\n");
	}
	l = stringFragList;
	printf("\nstring frag \n");
	for (; l; l = l->tail) {
		printf("name:%s,   string: %s\n", S_name(l->head->u.stringg.label),l->head->u.stringg.str);
	}
}
T_stmList canon(Tr_exp stm) {
	return C_traceSchedule(C_basicBlocks(C_linearize(unNx(stm))));
}