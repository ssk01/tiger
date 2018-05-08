#pragma once
#include "util.h"
#include "temp.h"
typedef struct F_frame_ *F_frame;
typedef struct F_access_ *F_access;

typedef struct F_accessList *F_accessList;
struct F_accessList_ { F_access head; F_accessList tail; };
F_frame F_newFrame(Temp_label name, U_boolList formals);
Temp_label F_name(F_frame f);
F_accessList F_formals(F_frame f);
F_access F_allocLocal(F_frame f, bool escape);

struct F_access_ {
	enum {inFrame, inReg} kind;
	union { 
		int offset;
		Temp_temp reg;
	} u;
};

static F_access InFrame(int offset1);
static F_access InReg(Temp_temp reg);