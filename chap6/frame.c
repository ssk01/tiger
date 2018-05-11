#include "frame.h"

const int FRAME_WORD_SIZE = 4;
static const int F_MAX_REG = 6;

struct F_frame_ {
	Temp_label name;
	F_accessList formals;
	int local_count;
};


struct F_access_ {
	enum { inFrame, inReg } kind;
	union {
		int offset;
		Temp_temp reg;
	} u;
};


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
		return InFrame(FRAME_WORD_SIZE*f->local_count);
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

static F_accessList makeFormalAccessList(F_frame f, U_boolList formals) {
	F_accessList acl = NULL;
	F_access ac = NULL;
	int i = 0;
	for (; formals; formals = formals->tail) {

		if (!formals->head && i < 6) {
			ac = InReg(Temp_newtemp());
		}
		else {
			ac = InFrame(i*FRAME_WORD_SIZE);
		}
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