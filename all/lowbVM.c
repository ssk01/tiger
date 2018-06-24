#include "lowbVM.h"
#include "util.h"
#include "temp.h"
#include "symbol.h"
#include "table.h"
#include "frame.h"
#include <memory.h>
static  int text[1000];
static int i = 0;
static char *stk;
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
	EXIT,
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
void init_reg();
void init_vm() {
	memset(text, 0, sizeof(text));
	stk = malloc(4000);
	memset(stk, 0, 4000);
	init_reg();
}
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
typedef struct Val* varptr;
varptr VAL(kind k) {
	varptr r = checked_malloc(sizeof(*r));
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
void R_enter(S_table t, Temp_temp reg, varptr value) {
	TAB_enter(t, reg, value);
}
void lable_enter(S_table t, string s, varptr value) {
	TAB_enter(t, s, value);
}
varptr label_look(S_table t, string s) {
	return TAB_look(t, s);
}
varptr R_look(S_table t, Temp_temp reg) {
	return TAB_look(t, reg);
}
//LABEL
void addLabel(string s) {
	//label
	//:l1   <-pc
	text[i] = LABEL;
	i++;
	text[i] = (int)s;
	varptr v = VAL(_i);
	v->i = i;
	lable_enter(labelTable(), s, v);
	i++;
}
void addCall(string s) {
	text[i] = CALL;
	i++;
	text[i] = (int)s;
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
	varptr r = R_look(regTable(), reg);
	if (r == NULL ){
		r = VAL(_r);
		r->i = -111;
		R_enter(regTable(), reg, r);
	}
	text[i] = (int)r;
	i++;
}
void addConst(int intt) {
	varptr in = VAL(_i);
	in->i = intt;
	text[i] = in;
	i++;
}
void init_reg() {
	varptr fp = VAL(_r);
	R_enter(regTable(), F_FP(), fp);
	fp->i = 4000;
	printf("ebp: %x\n", fp);

	varptr sp = VAL(_r);
	sp->i = 4000;
	printf("esp: %x\n", sp);
	R_enter(regTable(), F_SP(), sp);
}
void show(Temp_temp reg) {
	printf("result %d", R_look(regTable(), reg)->i);
}
void sheax() {
	Temp_temp eax = F_RV();
	show(eax);
}
void vm(string beg) {
	int pc = 0;
	varptr start = label_look(labelTable(), beg);
	pc = start->i;
	while (pc<i) {
		pc += 1;
		INS ins = text[pc];
		pc += 1;
		switch (ins) {
		case MOVCONST2REG: {
			varptr res = text[pc];
			pc++;
			varptr lhs = text[pc];
			res->i = lhs->i;
			break;
		}
		case MOVREG2REG: {
			varptr res = text[pc];
			pc++;
			varptr lhs = text[pc];
			res->i = lhs->i;
			break;

		}
		case MOVREG2MEM: {
			//mov[ebp + -4], r102
			varptr lhs = text[pc];
			pc++;
			varptr offset = text[pc];
			pc++;
			varptr rhs = text[pc];
			*(int *)(stk + lhs->i + offset->i) = rhs->i;
			break;
		}
		case MOVMEM2REG: {
			//mov     r106, [ebp + -4]
			varptr lhs = text[pc];
			pc++;
			varptr rhs = text[pc];
			pc++;
			varptr offset = text[pc];
			lhs->i = *(int *)(stk + rhs->i + offset->i);
			break;

		}
		case SUB: {
			//add eax, r117 + 1
			varptr res = text[pc];
			assert(res->type != _i);
			pc++;
			varptr lhs = text[pc];
			pc++;
			varptr rhs = text[pc];
			//if (rhs->type = _i){}
			res->i = lhs->i - rhs->i;
			break;
		}
		case PUSH: {
			varptr  res = text[pc];
			varptr sp = R_look(regTable(), F_SP());
			sp->i-=4;
			*(int *)(stk + sp->i)= res->i;
			break;

		}
		case ADD: {
			varptr res = text[pc];
			assert(res->type != _i);
			pc++;
			varptr lhs = text[pc];
			pc++;
			varptr rhs = text[pc];
			res->i = lhs->i + rhs->i;
			break;

		}
		case CALL: {
			varptr sp = R_look(regTable(), F_SP());
			string label = text[pc];

			sp->i-=4;
			*(int *)(stk + sp->i) = pc;
			pc = label_look(labelTable(), label)->i;
			break;
		}
		case POP:{
			break;

		}
		case EXIT: {
			return;
		}
		case RET:{
			//--sp;
			//old ebp <-- old esp <-- ebp
			//
			//sheax();
			varptr fp = R_look(regTable(), F_FP());
			varptr sp = R_look(regTable(), F_SP());
			sp->i = fp->i;
			int oldfp = *(int *)(stk + fp->i);
			pc = *(int *)(stk+fp->i + 4);
			fp->i = oldfp;
			//pc returnPc
			break;

		}
		case JMP:{
			string label = text[pc];
			varptr offset = label_look(labelTable(), label);
			pc = offset->i;
			break;

		}
		case LABEL: {

			break;

		}
		}
	}
}

void testVM() {
	init_vm();
	Temp_temp r102 = Temp_newtemp();
	Temp_temp r103 = Temp_newtemp();
	Temp_temp r104 = Temp_newtemp();
	Temp_temp r105 = Temp_newtemp();
	Temp_temp r106 = Temp_newtemp();
	Temp_temp r114 = Temp_newtemp();
	Temp_temp ebp = F_FP();
	Temp_temp esp = F_SP();
	Temp_temp eax = F_RV();
	string beg = String("L7");
	string L5 = String("L5");
	string L8 = String("L8");
	string L9 = String("L9");
	addLabel(beg);
	add(SUB);
	addReg(esp);
	addReg(esp);
	addConst(48);
	add(MOVCONST2REG);
	addReg(r102);
	addConst(100);
	add(MOVREG2MEM);
	addReg(ebp);
	addConst(-4);
	addReg(r102);
	add(MOVMEM2REG);
	addReg(r104);
	addReg(ebp);
	addConst(-4);
	add(PUSH);
	addReg(r104);
	add(PUSH);
	addReg(ebp);
	addCall(L5);
	add(EXIT);
	addLabel(L5);
	addLabel(L9);
	add(PUSH);
	addReg(ebp);
	add(MOVREG2REG);
	addReg(ebp);
	addReg(esp);
	add(SUB);
	addReg(esp);
	addReg(esp);
	addConst(-48);
	add(MOVMEM2REG);
	addReg(r114);
	addReg(ebp);
	addConst(12);
	addJump(L8);
	addLabel(L8);
	add(MOVREG2REG);
	addReg(eax);
	addReg(r114);
	add(RET);
	vm(beg);
	show(eax);
	//add
	//add(push)
}