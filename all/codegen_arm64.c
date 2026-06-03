#include "codegen_arm64.h"
#include "assem.h"
#include "tree.h"
#include "frame.h"
#include "temp.h"
#include "util.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

static AS_instrList instrList = NULL, last = NULL;
static F_frame codegenFrame = NULL;

static Temp_temp munchExp(T_exp e);
static int is_extern_call(const char *name) {
	if (!name) return 0;
	if (name[0] == '_') name++;
	static const char *ext[] = {
		"printInt","print","malloc","initArray","stringEqual",
		"ord","getchar","chr","flush","exit", NULL
	};
	for (int i = 0; ext[i]; i++)
		if (strcmp(name, ext[i]) == 0) return 1;
	return 0;
}

static int has_static_link(const char *name) {
	if (!name) return 0;
	if (name[0] == '_') name++;
	static const char *no_sl[] = {"initArray", "malloc", "stringEqual", NULL};
	for (int i = 0; no_sl[i]; i++)
		if (strcmp(name, no_sl[i]) == 0) return 0;
	return 1;
}

static void emit(AS_instr instr) {
	if (!instrList) instrList = last = AS_InstrList(instr, NULL);
	else last = last->tail = AS_InstrList(instr, NULL);
}

static int count_args(T_expList elist) {
	int n = 0;
	for (; elist; elist = elist->tail) n++;
	return n;
}

static Temp_tempList munchArgs(T_expList elist) {
	assert(codegenFrame != NULL);
	char buf[200];
	if (!elist) return NULL;
	Temp_tempList tlist = munchArgs(elist->tail);
	Temp_temp e = munchExp(elist->head);
	char *name = Temp_look(Temp_name(), e);
	if (name && name[0] == 'L') {
		Temp_temp addr = Temp_newtemp();
		sprintf(buf, "adrp `d0, _%s@PAGE\n", name);
		emit(AS_Oper(String(buf), Temp_TempList(addr, NULL), NULL, NULL));
		sprintf(buf, "add `d0, `d0, _%s@PAGEOFF\n", name);
		emit(AS_Oper(String(buf), Temp_TempList(addr, NULL), Temp_TempList(addr, NULL), NULL));
		e = addr;
	}
	int tail_count = count_args(elist->tail);
	sprintf(buf, "str `s0, [sp, #%d]\n", tail_count * 8);
	emit(AS_Move(String(buf), NULL, Temp_TempList(e, NULL)));
	return Temp_TempList(e, tlist);
}

static void emit_extern_bridge(int nargs) {
	char buf[200];
	char *arg_regs[] = {"x0","x1","x2","x3","x4","x5","x6","x7"};
	for (int i = 0; i < nargs && i < 8; i++) {
		Temp_temp areg = Temp_newtemp();
		Temp_enter(Temp_name(), areg, String(arg_regs[i]));
		int offset = (nargs - 1 - i) * 8;
		sprintf(buf, "ldr `d0, [sp, #%d]\n", offset);
		emit(AS_Move(String(buf), Temp_TempList(areg, NULL), NULL));
	}
}

static Temp_temp munchExp(T_exp e) {
	char buf[200];
	switch (e->kind) {
	case T_BINOP: {
		char *op = NULL;
		switch (e->u.BINOP.op) {
		case T_plus: op = "add"; break;
		case T_minus: op = "sub"; break;
		case T_mul: op = "mul"; break;
		case T_div: op = "sdiv"; break;
		default: assert(0);
		}
		Temp_temp r = Temp_newtemp();
		if (e->u.BINOP.left->kind == T_CONST) {
			int n = e->u.BINOP.left->u.CONST;
			Temp_temp s0 = munchExp(e->u.BINOP.right);
			if (n == 0 && e->u.BINOP.op == T_minus) {
				sprintf(buf, "neg `d0, `s0\n");
				emit(AS_Oper(String(buf), Temp_TempList(r, NULL), Temp_TempList(s0, NULL), NULL));
			} else if (n == 0 && op[0] == 's') {
				sprintf(buf, "mov `d0, `s0\n");
				emit(AS_Move(String(buf), Temp_TempList(r, NULL), Temp_TempList(s0, NULL)));
			} else if (op[0] == 'm' || op[0] == 's') {
				Temp_temp rt = Temp_newtemp();
				sprintf(buf, "mov `d0, #%d\n", n);
				emit(AS_Move(String(buf), Temp_TempList(rt, NULL), NULL));
				sprintf(buf, "%s `d0, `s0, `s1\n", op);
				emit(AS_Oper(String(buf), Temp_TempList(r, NULL),
					Temp_TempList(s0, Temp_TempList(rt, NULL)), NULL));
			} else {
				sprintf(buf, "%s `d0, `s0, #%d\n", op, n);
				emit(AS_Oper(String(buf), Temp_TempList(r, NULL), Temp_TempList(s0, NULL), NULL));
			}
			return r;
		} else if (e->u.BINOP.right->kind == T_CONST) {
			int n = e->u.BINOP.right->u.CONST;
			Temp_temp s0 = munchExp(e->u.BINOP.left);
			if (n == 0 && op[0] == 's') {
				sprintf(buf, "mov `d0, `s0\n");
				emit(AS_Move(String(buf), Temp_TempList(r, NULL), Temp_TempList(s0, NULL)));
			} else if (op[0] == 'm' || op[0] == 's') {
				if (n == 1 && op[0] == 'm') {
					sprintf(buf, "mov `d0, `s0\n");
					emit(AS_Move(String(buf), Temp_TempList(r, NULL), Temp_TempList(s0, NULL)));
				} else {
					Temp_temp rt = Temp_newtemp();
					sprintf(buf, "mov `d0, #%d\n", n);
					emit(AS_Move(String(buf), Temp_TempList(rt, NULL), NULL));
					sprintf(buf, "%s `d0, `s0, `s1\n", op);
					emit(AS_Oper(String(buf), Temp_TempList(r, NULL),
						Temp_TempList(s0, Temp_TempList(rt, NULL)), NULL));
				}
			} else {
				sprintf(buf, "%s `d0, `s0, #%d\n", op, n);
				emit(AS_Oper(String(buf), Temp_TempList(r, NULL), Temp_TempList(s0, NULL), NULL));
			}
			return r;
		} else {
			Temp_temp s0 = munchExp(e->u.BINOP.left);
			Temp_temp s1 = munchExp(e->u.BINOP.right);
			sprintf(buf, "%s `d0, `s0, `s1\n", op);
			emit(AS_Oper(String(buf), Temp_TempList(r, NULL),
				Temp_TempList(s0, Temp_TempList(s1, NULL)), NULL));
			return r;
		}
	}
	case T_MEM: {
		T_exp mem = e->u.MEM;
		Temp_temp r = Temp_newtemp();
		if (mem->kind == T_BINOP && mem->u.BINOP.op == T_plus) {
			if (mem->u.BINOP.left->kind == T_CONST) {
				Temp_temp s0 = munchExp(mem->u.BINOP.right);
				int n = mem->u.BINOP.left->u.CONST;
				sprintf(buf, "ldr `d0, [`s0, #%d]\n", n);
				emit(AS_Move(String(buf), Temp_TempList(r, NULL), Temp_TempList(s0, NULL)));
				return r;
			} else if (mem->u.BINOP.right->kind == T_CONST) {
				Temp_temp s0 = munchExp(mem->u.BINOP.left);
				int n = mem->u.BINOP.right->u.CONST;
				sprintf(buf, "ldr `d0, [`s0, #%d]\n", n);
				emit(AS_Move(String(buf), Temp_TempList(r, NULL), Temp_TempList(s0, NULL)));
				return r;
			}
		}
		Temp_temp s0 = munchExp(mem);
		sprintf(buf, "ldr `d0, [`s0]\n");
		emit(AS_Move(String(buf), Temp_TempList(r, NULL), Temp_TempList(s0, NULL)));
		return r;
	}
	case T_TEMP:
		return e->u.TEMP;
	case T_ESEQ:
		assert(0);
		return NULL;
	case T_NAME: {
		Temp_temp r = Temp_newtemp();
		if (!F_tempMap) F_tempMap = Temp_name();
		Temp_enter(F_tempMap, r, Temp_labelstring(e->u.NAME));
		return r;
	}
	case T_CONST: {
		Temp_temp r = Temp_newtemp();
		sprintf(buf, "mov `d0, #%d\n", e->u.CONST);
		emit(AS_Move(String(buf), Temp_TempList(r, NULL), NULL));
		return r;
	}
	case T_CALL: {
		Temp_temp r = munchExp(e->u.CALL.fun);
		char *fname = Temp_look(Temp_name(), r);
		T_expList args = e->u.CALL.args;
		int is_ext = is_extern_call(fname);
		if (is_ext && has_static_link(fname) && args) args = args->tail;
		int nargs = count_args(args);
		int arg_bytes = nargs * 8;
		int pad = 0;
		if (arg_bytes) {
			int after = (arg_bytes + 15) & ~15;
			pad = after - arg_bytes;
			arg_bytes = after;
			sprintf(buf, "sub sp, sp, #%d\n", arg_bytes);
			emit(AS_Oper(String(buf), Temp_TempList(F_SP(), NULL), NULL, NULL));
		}
		Temp_tempList list = munchArgs(args);
		if (is_ext) emit_extern_bridge(nargs);
		if (fname && !is_ext && fname[0] != '_') {
			char lbuf[64]; sprintf(lbuf, "_%s", fname);
			Temp_enter(F_tempMap, r, String(lbuf));
		}
		if (is_ext && fname) {
			char lbuf[64]; sprintf(lbuf, "_%s", fname);
			Temp_enter(F_tempMap, r, String(lbuf));
		}
		sprintf(buf, "bl `s0\n");
		emit(AS_Oper(String(buf), NULL, Temp_TempList(r, list), NULL));
		if (arg_bytes) {
			sprintf(buf, "add sp, sp, #%d\n", arg_bytes);
			emit(AS_Oper(String(buf), Temp_TempList(F_SP(), NULL), NULL, NULL));
		}
		return F_RV();
	}
	default:
		break;
	}
	return NULL;
}

static Temp_temp munchStm(T_stm stm) {
	char buf[200];
	switch (stm->kind) {
	case T_SEQ:
		munchStm(stm->u.SEQ.left);
		munchStm(stm->u.SEQ.right);
		break;
	case T_LABEL: {
		char lbuf[64];
		sprintf(lbuf, "_%s:\n", Temp_labelstring(stm->u.LABEL));
		emit(AS_Label(String(lbuf), stm->u.LABEL));
		return F_VOID();
	}
	case T_JUMP: {
		Temp_temp r = munchExp(stm->u.JUMP.exp);
		char *name = Temp_look(Temp_name(), r);
		if (name && name[0] == 'L')
			sprintf(buf, "b _%s\n", name);
		else
			sprintf(buf, "br `s0\n");
		emit(AS_Oper(String(buf), NULL,
			Temp_TempList(r, NULL), AS_Targets(stm->u.JUMP.jumps)));
		return F_VOID();
	}
	case T_CJUMP: {
		Temp_temp left = munchExp(stm->u.CJUMP.left);
		Temp_temp right = munchExp(stm->u.CJUMP.right);
		sprintf(buf, "cmp `s0, `s1\n");
		emit(AS_Oper(String(buf), NULL,
			Temp_TempList(left, Temp_TempList(right, NULL)), NULL));
		char *cond = NULL;
		switch (stm->u.CJUMP.op) {
		case T_eq: cond = "b.eq"; break;
		case T_ne: cond = "b.ne"; break;
		case T_lt: cond = "b.lt"; break;
		case T_gt: cond = "b.gt"; break;
		case T_le: cond = "b.le"; break;
		case T_ge: cond = "b.ge"; break;
		default: assert(0);
		}
		sprintf(buf, "%s _`j0\n", cond);
		emit(AS_Oper(String(buf), NULL, NULL,
			AS_Targets(Temp_LabelList(stm->u.CJUMP.true, NULL))));
		break;
	}
	case T_MOVE: {
		T_exp dst = stm->u.MOVE.dst;
		T_exp src = stm->u.MOVE.src;
		if (dst->kind == T_TEMP) {
			Temp_temp d0 = munchExp(dst);
			Temp_temp s0 = munchExp(src);
			char *sname = Temp_look(Temp_name(), s0);
			if (sname && sname[0] == 'L') {
				Temp_temp addr = Temp_newtemp();
				sprintf(buf, "adrp `d0, _%s@PAGE\n", sname);
				emit(AS_Oper(String(buf), Temp_TempList(addr, NULL), NULL, NULL));
				sprintf(buf, "add `d0, `d0, _%s@PAGEOFF\n", sname);
				emit(AS_Oper(String(buf), Temp_TempList(addr, NULL), Temp_TempList(addr, NULL), NULL));
				sprintf(buf, "mov `d0, `s0\n");
				emit(AS_Move(String(buf), Temp_TempList(d0, NULL), Temp_TempList(addr, NULL)));
			} else {
				sprintf(buf, "mov `d0, `s0\n");
				emit(AS_Move(String(buf), Temp_TempList(d0, NULL), Temp_TempList(s0, NULL)));
			}
			return d0;
		} else if (dst->kind == T_MEM) {
			Temp_temp s0, s1;
			int n;
			if (dst->u.MEM->kind == T_BINOP &&
			    dst->u.MEM->u.BINOP.op == T_plus &&
			    dst->u.MEM->u.BINOP.right->kind == T_CONST) {
				s0 = munchExp(dst->u.MEM->u.BINOP.left);
				s1 = munchExp(src);
				n = dst->u.MEM->u.BINOP.right->u.CONST;
			} else if (dst->u.MEM->kind == T_BINOP &&
			           dst->u.MEM->u.BINOP.op == T_plus &&
			           dst->u.MEM->u.BINOP.left->kind == T_CONST) {
				s0 = munchExp(dst->u.MEM->u.BINOP.right);
				s1 = munchExp(src);
				n = dst->u.MEM->u.BINOP.left->u.CONST;
			} else {
				s0 = munchExp(dst->u.MEM);
				s1 = munchExp(src);
				sprintf(buf, "str `s1, [`s0]\n");
				emit(AS_Move(String(buf), NULL,
					Temp_TempList(s0, Temp_TempList(s1, NULL))));
				break;
			}
			sprintf(buf, "str `s1, [`s0, #%d]\n", n);
			emit(AS_Move(String(buf), NULL,
				Temp_TempList(s0, Temp_TempList(s1, NULL))));
		} else assert(0);
		break;
	}
	case T_EXP:
		return munchExp(stm->u.EXP);
	default:
		assert(0);
	}
	return NULL;
}

AS_instrList F_codegen_arm64(F_frame frame, T_stmList stmList, int main_flag) {
	AS_instrList asList = NULL;
	T_stmList sList = stmList;
	codegenFrame = frame;
	char buf[200];
	assert(sList->head != NULL);

	Temp_enter(Temp_name(), F_FP(), "x29");
	Temp_enter(Temp_name(), F_SP(), "sp");
	Temp_enter(Temp_name(), F_RV(), "x0");
	Temp_enter(Temp_name(), F_VOID(), "xzr");

	sprintf(buf, "_%s:\n", S_name(F_name(frame)));
	emit(AS_Label(String(buf), F_name(frame)));

	int stk_size = stack_size(frame);
	int npushes = (main_flag == 0) ? 3 : 2;
	int total = npushes * 8 + stk_size;
	total = (total + 15) & ~15;

	sprintf(buf, "sub sp, sp, #%d\n", total);
	emit(AS_Oper(String(buf), Temp_TempList(F_SP(), NULL), NULL, NULL));
	if (main_flag == 0) {
		sprintf(buf, "str xzr, [sp, #16]\n");
		emit(AS_Move(String(buf), NULL, NULL));
	}
	sprintf(buf, "str x30, [sp, #8]\n");
	emit(AS_Move(String(buf), NULL, NULL));
	sprintf(buf, "str x29, [sp]\n");
	emit(AS_Move(String(buf), NULL, NULL));
	sprintf(buf, "mov x29, sp\n");
	emit(AS_Oper(String(buf), Temp_TempList(F_FP(), NULL), Temp_TempList(F_SP(), NULL), NULL));

	Temp_temp rv = F_RV();
	for (; sList; sList = sList->tail) {
		Temp_temp t = munchStm(sList->head);
		if (t != F_VOID()) rv = t;
	}

	if (main_flag == 0) {
		sprintf(buf, "mov x0, #0\n");
		emit(AS_Oper(String(buf), Temp_TempList(F_RV(), NULL), NULL, NULL));
		sprintf(buf, "bl _Tiger_exit\n");
		emit(AS_Oper(String(buf), NULL, NULL, NULL));
	} else {
		sprintf(buf, "mov sp, x29\n");
		emit(AS_Oper(String(buf), NULL, NULL, NULL));
		sprintf(buf, "ldr x29, [sp]\n");
		emit(AS_Oper(String(buf), Temp_TempList(F_FP(), NULL), NULL, NULL));
		sprintf(buf, "ldr x30, [sp, #8]\n");
		emit(AS_Oper(String(buf), NULL, NULL, NULL));
		sprintf(buf, "add sp, sp, #16\n");
		emit(AS_Oper(String(buf), Temp_TempList(F_SP(), NULL), NULL, NULL));
		sprintf(buf, "ret\n");
		emit(AS_Oper(String(buf), NULL, NULL, NULL));
	}

	asList = instrList;
	instrList = last = NULL;
	return asList;
}
