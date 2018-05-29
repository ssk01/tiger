#include "lowbVM.h"
#include "util.h"
#include "temp.h"
#include "symbol.h"
#include "table.h"
#include "frame.h"
static  int text[1000];
static int i = 0;
static int *stk;
typedef
enum {
	MOVCONST2REG,
	MOVREG2REG,
	MOVREG2MEM,
	MOVMEM2REG,
	SUB,
	PUSH,
	ADD,
	POP,
	RET,
	JMP,
	LABEL,
	CALL,
} INS;


static S_table t = NULL;
S_table regTable() {
	if (t == NULL) {
		t = S_empty();
	}
	return t;
}
static S_table l = NULL;
S_table labelTable() {
	if (l == NULL) {
		l = S_empty();
	}
	return l;
}
//struct Reg {
//	int i;
//};
//typedef struct Reg* Regptr;
typedef
enum {
	_i,
	_r,
}kind;
struct Val {
	int type;
	//int i;
	int i;
};
typedef struct Val* ValPtr;
ValPtr VAL(kind k) {
	ValPtr r = checked_malloc(sizeof(*r));
	r->type = k;
	return r;
}
//struct _int {
//	int i;
//};
//typedef struct _int* Int;
//Int INT(int i) {
//	Int t = checked_malloc(sizeof(*t));
//	t->i = i;
//	return t;
//}
//Regptr REG() {
//	Regptr r = checked_malloc(sizeof(*r));
//	return r;
//}
void R_enter(S_table t, Temp_temp reg, ValPtr value) {
	TAB_enter(t, reg, value);
}
void lable_enter(S_table t, string s, ValPtr value) {
	TAB_enter(t, s, value);
}
ValPtr label_look(S_table t, string s) {
	return TAB_look(t, s);
}
ValPtr R_look(S_table t, Temp_temp reg) {
	return TAB_look(t, reg);
}
//LABEL
void addLabel(string s) {
	//label
	//:l1   <-pc
	text[i] = LABEL;
	i++;
	text[i] = (int)s;
	ValPtr v = VAL(_i);
	v->i = i;
	lable_enter(labelTable(), s, v);
	i++;
}
void addJump(string s) {
	text[i] = JMP;
	i++;
	text[i] = (int)s;
	i++;
}
void add(INS ins) {
	text[i] = ins;
	assert(ins != LABEL);
	i++;
}
void addReg(Temp_temp reg) {
	//S_enter(S_table t, S_symbol sym, void *value);
	ValPtr r = R_look(regTable(), reg);
	if (r == NULL ){
		ValPtr r = VAL(_r);
		r->i = -1;
		R_enter(regTable(), reg, r);
	}
	text[i] = (int)r;
	i++;
}
void addConst(int intt) {
	text[i] = intt;
	i++;
}
void init() {
	ValPtr fp = VAL(_r);
	R_enter(regTable(), F_FP(), fp);
	fp->i = 4000;
	ValPtr sp = VAL(_r);
	sp->i = 4000;
	R_enter(regTable(), F_SP(), sp);
}
void vm() {
	int pc = 0;
	ValPtr i = label_look(labelTable(), String("l7"));
	pc = i->i;
	while (text[pc] != RET) {
		pc += 1;
		INS ins = text[pc];
		pc += 1;
		switch (ins) {
		case MOVCONST2REG: {
			ValPtr res = text[pc];
			pc++;
			ValPtr lhs = text[pc];
			res->i = lhs->i;
			break;
		}
		case MOVREG2REG: {
			ValPtr res = text[pc];
			pc++;
			ValPtr lhs = text[pc];
			res->i = lhs->i;
			break;

		}
		case MOVREG2MEM: {
			//mov[ebp + -4], r102
			ValPtr lhs = text[pc];
			pc++;
			int offset = text[pc];
			ValPtr rhs = text[pc];
			pc++;
			stk[lhs->i + offset] = rhs->i;
			break;
		}
		case MOVMEM2REG: {
			//mov     r106, [ebp + -4]
			ValPtr lhs = text[pc];
			pc++;
			ValPtr rhs = text[pc];
			pc++;
			int offset = text[pc];
			lhs->i = stk[rhs->i + offset];
			break;

		}
		case SUB: {
			//add eax, r117 + 1
			ValPtr res = text[pc];
			assert(res->type != _i);
			pc++;
			ValPtr lhs = text[pc];
			pc++;
			ValPtr rhs = text[pc];
			//if (rhs->type = _i){}
			res->i = lhs->i - rhs->i;
			break;
		}
		case PUSH: {
			ValPtr  res = text[pc];
			ValPtr sp = R_look(regTable(), F_SP());
			sp->i--;
			stk[sp->i] = res->i;
			break;

		}
		case ADD: {
			ValPtr res = text[pc];
			assert(res->type != _i);
			pc++;
			ValPtr lhs = text[pc];
			pc++;
			ValPtr rhs = text[pc];
			res->i = lhs->i + rhs->i;
			break;

		}
		case CALL: {
			ValPtr sp = R_look(regTable(), F_SP());
			string label = text[pc];

			sp->i--;
			stk[sp->i] = pc;
			pc = label_look(labelTable(), label);
			break;
		}
		case POP:{
			break;

		}
		case RET:{
			//--sp;
			//old ebp <-- old esp <-- ebp
			//
			ValPtr fp = R_look(regTable(), F_FP());
			ValPtr sp = R_look(regTable(), F_SP());
			sp->i = fp->i;
			int oldfp = stk[fp->i];
			pc = stk[fp->i + 1];
			fp->i = oldfp;
			//pc returnPc
			break;

		}
		case JMP:{
			string label = text[pc];
			ValPtr i = label_look(labelTable(), label);
			pc = i->i;
			break;

		}
		case LABEL: {

			break;

		}
		}
	}
}