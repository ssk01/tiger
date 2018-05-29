#include "frame.h"

const int FRAME_WORD_SIZE = 4;
static const int F_MAX_REG = 6;

struct F_frame_ {
	Temp_label name;
	F_accessList formals;
	int local_count;
};

Temp_map F_tempMap = NULL;
struct F_access_ {
	enum { inFrame, inReg } kind;
	union {
		int offset;
		Temp_temp reg;
	} u;
};
int F_escape(F_access ac) {
	assert(ac != NULL);
	if (ac->kind == inFrame) {
		return 0;
	}
	return 1;
}
static void F_add_to_map(string str, Temp_temp temp);
static F_accessList makeFormalAccessList(F_frame f, U_boolList formals);
static F_access InFrame(int offset1);
static F_access InReg(Temp_temp reg);
F_frame F_newFrame(Temp_label name, U_boolList formals);
Temp_label F_name(F_frame f) {
	return f->name;
}
F_accessList F_formals(F_frame f) {
	return f->formals;
}

F_access F_allocLocal(F_frame f, bool escape) {
	f->local_count++;
	if (escape) {
		return InFrame(-FRAME_WORD_SIZE*f->local_count);
	}
	else {
		return InReg(Temp_newtemp());
	}
}

F_accessList F_AccessList(F_access head,F_accessList tail) {
	F_accessList acl = checked_malloc(sizeof(*acl));
	acl->head = head;
	acl->tail = tail;
	return acl;
}
F_frame F_newFrame(Temp_label name, U_boolList formals) {
	F_frame f = checked_malloc(sizeof(*f));
	f->name = name;
	f->formals = makeFormalAccessList(f, formals);
	f->local_count = 0;
	return f;
}
static Temp_tempList F_make_arg_regs(void)
{
	Temp_temp eax = Temp_newtemp(), ebx = Temp_newtemp(),
		ecx = Temp_newtemp(), edx = Temp_newtemp(), edi = Temp_newtemp(),
		esi = Temp_newtemp();
	F_add_to_map("eax", eax); F_add_to_map("ebx", ebx); F_add_to_map("ecx", ecx);
	F_add_to_map("edx", edx); F_add_to_map("edi", edi); F_add_to_map("esi", esi);
	return Temp_TempList(eax, Temp_TempList(ebx, Temp_TempList(ecx, Temp_TempList(edx, Temp_TempList(edi, Temp_TempList(esi, NULL))))));
}

static Temp_temp rv = NULL;
static Temp_temp v = NULL;
Temp_temp F_VOID(void)
{
	if (!v) {
		v = Temp_newtemp();
		F_add_to_map("void", v);
	}
	return v;
}
Temp_temp F_RV(void)
{
	if (!rv) {
		rv = Temp_newtemp();
		F_add_to_map("eax", rv);
	}
	return rv;
}

static Temp_tempList F_make_caller_saves(void)
{
	return Temp_TempList(F_RV(), F_make_arg_regs());
}
Temp_tempList F_caller_saves(void)
{
	static Temp_tempList caller_saves = NULL;
	if (!caller_saves) {
		caller_saves = F_make_caller_saves();
	}
	return caller_saves;
}



static F_accessList makeFormalAccessList(F_frame f, U_boolList formals) {
	F_accessList acl = NULL;
	F_access ac = NULL;
	int i = 0;
	for (; formals; formals = formals->tail) {

	/*	if (!formals->head && i < 6) {
			ac = InReg(Temp_newtemp());
		}
		else {*/
			ac = InFrame((2+i)*FRAME_WORD_SIZE);
		//}
		acl = F_AccessList(ac, acl);
		i++;
	}
	return acl;
}
static F_access InFrame(int offset1) {
	F_access a = checked_malloc(sizeof(*a));
	a->kind = inFrame;
	a->u.offset = offset1;
	return a;
}
static F_access InReg(Temp_temp reg) {
	F_access a = checked_malloc(sizeof(*a));
	a->kind = inReg;
	a->u.reg = reg;
	return a;
}
static Temp_temp fp = NULL;
static Temp_temp sp = NULL;
Temp_temp F_FP() {
	if (fp == NULL) {
		fp = Temp_newtemp();
		F_add_to_map("ebp", fp);
	}
	return fp;
}
Temp_temp F_SP() {
	if (sp == NULL) {
		sp = Temp_newtemp();
		F_add_to_map("esp", sp);
	}
	return sp;
}
static void F_add_to_map(string str, Temp_temp temp)
{
	if (!F_tempMap) {
		F_tempMap = Temp_name();
	}
	Temp_enter(F_tempMap, temp, str);
}
T_exp F_Exp(F_access acc, T_exp framePtr) {
	return T_Mem(T_Binop(T_plus, framePtr, T_Const(acc->u.offset)));
}

T_exp ExternCall(string s, T_expList args) {
	return T_Call(T_Name(Temp_namedlabel(s)), args);
}
//void dis_ac(F_access a) {
//	switch (a->kind)
//	{
//	case inFrame: {
//		printf("stack: %i\n", a->u.offset);
//		break;
//	}
//	case inReg: {
//		printf("reg: %d\n", temp_tempint)
//	}
//	default:
//		assert(0);
//		break;
//	}
//}