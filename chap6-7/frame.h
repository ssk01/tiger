#pragma once
#include "util.h"
#include "temp.h"
#include "tree.h"
typedef struct F_frame_ *F_frame;
typedef struct F_access_ *F_access;
typedef struct F_accessList_ *F_accessList;
extern Temp_map F_tempMap;
struct F_accessList_ { F_access head; F_accessList tail; };
F_frame F_newFrame(Temp_label name, U_boolList formals);
Temp_label F_name(F_frame f);
F_accessList F_formals(F_frame f);
F_access F_allocLocal(F_frame f, bool escape);
F_accessList F_AccessList(F_access head, F_accessList tail);
Temp_temp F_FP();
T_exp F_Exp(F_access acc, T_exp framePtr);
T_exp ExternCall(string s, T_expList args);