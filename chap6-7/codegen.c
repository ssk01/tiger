#include "codegen.h"
#include "assem.h"
#include "tree.h"
#include "frame.h"
//static F_frame CODEGEN_frame = NULL; // for munchArgs
static AS_instrList instrList = NULL, last = NULL;
//Temp_map F_tempMap = NULL;
F_frame codegenFrame = NULL;

static Temp_temp munchExp(T_exp e);
static void emit(AS_instr instr)
{
	//printf("fuck");
	if (!instrList) instrList = last = AS_InstrList(instr, NULL);
	else last = last->tail = AS_InstrList(instr, NULL);
}
//char *AS_format(char *des, char *format, )
static char *register_names[] = { "targ1", "targ2", "targ3", "targ4", "targ5", "targ6" };
static unsigned int reg_count = 0;
static Temp_tempList munchArgs(unsigned int n, T_expList elist) {
	assert(codegenFrame != NULL);
	char assem_string[100];
	if (!elist ) return NULL;
	Temp_tempList tlist = munchArgs(n + 1, elist->tail);
	Temp_temp e = munchExp(elist->head);
	//这个和frame里面分配参数情况不一样，而且reg_count数量还需要考虑。
	sprintf(assem_string, "push `s0\n");
	emit(AS_Oper(String(assem_string), NULL, Temp_TempList(e, NULL), NULL));
	return Temp_TempList(e, tlist);
}
static Temp_temp munchExp(T_exp e) {
	char assem_string[100];
	//String()
	switch (e->kind){
		case T_BINOP: {
			char *op = NULL, *sign = NULL;
			switch (e->u.BINOP.op) {
			case T_plus:
				op = "add"; sign = "+"; break;
			case T_minus:
				op = "sub"; sign = "-"; break;
			case T_mul:
				op = "mul"; sign = "*"; break;
			case T_div:
				op = "div"; sign = "/"; break;
			default:
				assert(0 && "Invalid operator");
			}
			//if (1)
			Temp_temp r = F_RV();
				
			if (e->u.BINOP.left->kind == T_CONST) {
				T_exp e2 = e->u.BINOP.right;
				int n = e->u.BINOP.left->u.CONST;
				Temp_temp s0 = munchExp(e2);
				sprintf(assem_string, "%s `d0,`s0%s%d\n", op, sign, n);
				emit(AS_Oper(String(assem_string), Temp_TempList(r, NULL), Temp_TempList(s0, NULL), NULL));
				return r;
			}
			else if (e->u.BINOP.right->kind == T_CONST) {
				T_exp e2 = e->u.BINOP.left;
				int n = e->u.BINOP.right->u.CONST;
				Temp_temp s0 = munchExp(e2);
				sprintf(assem_string, "%s `d0,`s0%s%d\n", op, sign, n);
				emit(AS_Oper(String(assem_string), Temp_TempList(r, NULL), Temp_TempList(s0, NULL), NULL));
				return r;
			}
			else {
				T_exp e1 = e->u.BINOP.left;
				T_exp e2 = e->u.BINOP.right;
				//emit(A)
				//AS_Oper(as
				Temp_temp s0 = munchExp(e1);
				Temp_temp s1 = munchExp(e2);
				sprintf(assem_string, "%s `d0,`s0%s`s1\n", op, sign);
				emit(AS_Oper(String(assem_string), Temp_TempList(r, NULL), Temp_TempList(s0, Temp_TempList(s1, NULL)), NULL));
				return r;
			}
		}
		case T_MEM: {
			T_exp mem = e->u.MEM;
			Temp_temp r = Temp_newtemp();
			if (mem->kind == T_BINOP && mem->u.BINOP.op == T_plus) {
				if (mem->u.BINOP.left->kind == T_CONST) {
					T_exp e2 = mem->u.BINOP.right;
					int n = mem->u.BINOP.left->u.CONST;
					Temp_temp s0 = munchExp(e2);
					sprintf(assem_string, "mov	`d0,[`s0+%d]\n", n);
					emit(AS_Move(String(assem_string), Temp_TempList(r, NULL), Temp_TempList(s0, NULL)));
					return r;
				}
				else if (mem->u.BINOP.right->kind == T_CONST) {
					T_exp e2 = mem->u.BINOP.left;
					int n = mem->u.BINOP.right->u.CONST;
					Temp_temp s0 = munchExp(e2);
					sprintf(assem_string, "mov	`d0,[`s0+%d]\n", n);
					emit(AS_Move(String(assem_string), Temp_TempList(r, NULL), Temp_TempList(s0, NULL)));
					return r;
				}
				else {
					Temp_temp s0 = munchExp(mem);
					sprintf(assem_string, "mov	`d0,[`s0]\n");
					emit(AS_Move(String(assem_string), Temp_TempList(r, NULL), Temp_TempList(s0, NULL)));
					return r;
				}
				//assert(0);
			}
			else if (mem->kind == T_CONST) {
				Temp_temp r = Temp_newtemp();
				int n = mem->u.CONST;
				sprintf(assem_string, "mov	`d0,[%d]\n", n);
				emit(AS_Move(String(assem_string),
					Temp_TempList(r, NULL), NULL));
				return r;
			}
			else {
				// mem(e1)
				assert(mem->kind == T_MEM);
				Temp_temp r = Temp_newtemp();
				T_exp e1 = mem->u.MEM;
				Temp_temp s0 = munchExp(e1);
				sprintf(assem_string, "mov	`d0,[`s0]\n");
				emit(AS_Move(String(assem_string), Temp_TempList(r, NULL), Temp_TempList(s0, NULL)));
				return r;

			}

		}
		case T_TEMP: {
			// TEMP(t)
			return e->u.TEMP;
		}
		case T_ESEQ: {
			assert(0);
			munchExp(e->u.ESEQ.stm);
			return munchExp(e->u.ESEQ.exp);
		}
		case T_NAME: {
			Temp_temp r = Temp_newtemp();
			/*F_ass*/
			if (!F_tempMap) {
				F_tempMap = Temp_name();
			}
			Temp_enter(F_tempMap, r, Temp_labelstring(e->u.NAME));
			return r;
		}
		case T_CONST: {
			Temp_temp r = Temp_newtemp();
			int n = e->u.CONST;
			sprintf(assem_string, "mov	`d0,%d\n", n);
			emit(AS_Move(String(assem_string),
				Temp_TempList(r, NULL), NULL));
			return r;
		}
		case T_CALL: {
			Temp_temp r = munchExp(e->u.CALL.fun);
			Temp_tempList list = munchArgs(0, e->u.CALL.args);
			sprintf(assem_string, "call	`s0\n");

			emit(AS_Oper(String(assem_string), F_caller_saves(), Temp_TempList(r, list), NULL));
			return F_RV();
		}

	default:
		break;
	}
}
static Temp_temp munchStm(T_stm stm) {
	char assem_string[100];

	switch (stm->kind)
	{
	case T_SEQ: {
		munchStm(stm->u.SEQ.left);
		munchStm(stm->u.SEQ.right);
		break;
	}
	case T_LABEL: {
		sprintf(assem_string, "%s:\n", Temp_labelstring(stm->u.LABEL));
		emit(AS_Label(String(assem_string), stm->u.LABEL));
		return F_VOID();
		break;
	}
	case T_JUMP: {
		Temp_temp r = munchExp(stm->u.JUMP.exp);
		sprintf(assem_string, "jmp	`d0\n");
		emit(AS_Oper(String(assem_string), Temp_TempList(r, NULL), NULL, AS_Targets(stm->u.JUMP.jumps)));
		return F_VOID();
		break;

	}
	case T_CJUMP: {
		Temp_temp left = munchExp(stm->u.CJUMP.left);
		Temp_temp right = munchExp(stm->u.CJUMP.right);
		sprintf(assem_string, "cmp	`s0, `s1\n");
		emit(AS_Oper(String(assem_string), NULL, Temp_TempList(left, Temp_TempList(right, NULL)), NULL));
		
		char *instr = NULL;
		switch (stm->u.CJUMP.op) {
		case T_eq:
			instr = "je"; break;
		case T_ne:
			instr = "jne"; break;
		case T_lt:
			instr = "jl"; break;
		case T_gt:
			instr = "jg"; break;
		case T_le:
			instr = "jle"; break;
		case T_ge:
			instr = "jge"; break;
		default: assert(0);
		}
		sprintf(assem_string, "%s `j0\n", instr);
		emit(AS_Oper(String(assem_string), NULL, NULL, AS_Targets(Temp_LabelList(stm->u.CJUMP.true, NULL))));
		break;

	}
	case T_MOVE: {
		T_exp dst = stm->u.MOVE.dst;
		T_exp src = stm->u.MOVE.src;
		if (dst->kind == T_TEMP) {
			Temp_temp d0 = munchExp(dst);
			Temp_temp s0 = munchExp(src);
			sprintf(assem_string, "mov `d0, `s0\n");
			if (d0->num == 101) {
				printf("101");
			}
			emit(AS_Move(String(assem_string), Temp_TempList(d0, NULL), Temp_TempList(s0, NULL)));
			return d0;
		}
		else if (dst->kind == T_MEM) {
			if (dst->u.MEM->kind == T_BINOP &&
				dst->u.MEM->u.BINOP.op == T_plus &&
				dst->u.MEM->u.BINOP.right->kind == T_CONST) {
				Temp_temp s0 = munchExp(dst->u.MEM->u.BINOP.left);
				Temp_temp s1 = munchExp(src);
				int n = dst->u.MEM->u.BINOP.right->u.CONST;
				sprintf(assem_string, "mov [`s0+%d], `s1\n", n);
				emit(AS_Move(String(assem_string), NULL, Temp_TempList(s0, Temp_TempList(s1, NULL))));
			}
			else if (dst->u.MEM->kind == T_BINOP &&
				dst->u.MEM->u.BINOP.op == T_plus &&
				dst->u.MEM->u.BINOP.left->kind == T_CONST) {
				Temp_temp s0 = munchExp(dst->u.MEM->u.BINOP.right);
				Temp_temp s1 = munchExp(src);
				int n = dst->u.MEM->u.BINOP.left->u.CONST;
				sprintf(assem_string, "mov [`s0+%d], `s1\n", n);
				emit(AS_Move(String(assem_string), NULL, Temp_TempList(s0, Temp_TempList(s1, NULL))));
			}
			else if (dst->u.MEM->kind == T_CONST) {
				Temp_temp s0 = munchExp(src);
				int n = dst->u.MEM->u.CONST;
				//confuse
				sprintf(assem_string, "mov [%d], `s0\n", n);
				emit(AS_Move(String(assem_string), NULL, Temp_TempList(s0, NULL)));
				
			}
			else if (src->kind == T_MEM) {
				Temp_temp s0 = munchExp(dst->u.MEM);
				Temp_temp s1 = munchExp(src->u.MEM);
				sprintf(assem_string, "mov [s0], `s1\n");
				emit(AS_Move(String(assem_string), NULL, Temp_TempList(s0, Temp_TempList(s1, NULL))));
			}
			else {
				Temp_temp s0 = munchExp(dst->u.MEM);
				Temp_temp s1 = munchExp(src);
				sprintf(assem_string, "mov [`s0], `s1\n");
				emit(AS_Move(String(assem_string), NULL, Temp_TempList(s0, Temp_TempList(s1, NULL))));
			}

		}
		else
			assert(0);
		break;

	}
	case T_EXP: {
		return munchExp(stm->u.EXP);
		break;
	}
	//case T_ESEQ: {
	//	printf("fck");
	//}
	default:
		assert(0);
		break;
	}
	return NULL;
}

AS_instrList F_codegen(F_frame frame, T_stmList stmList, int main) {
	AS_instrList asList = NULL;
	T_stmList sList = stmList;
	codegenFrame = frame;
	int first = 1;
	char assem_string[100];
	assert(sList->head != NULL);
	//munchStm(sList->head);
	//sList = sList->tail;
	if (main == 0) {
		sprintf(assem_string, "push -1 #fake address for fuck static link\n");
		emit(AS_Oper(String(assem_string), NULL, NULL, NULL));
		sprintf(assem_string, "push -1 #fake address for end\n");
		emit(AS_Oper(String(assem_string), NULL,  NULL, NULL));
	}
	sprintf(assem_string, "push `s0\n");
	emit(AS_Oper(String(assem_string), NULL, Temp_TempList(F_FP(), NULL), NULL));
	sprintf(assem_string, "mov `d0, `s0\n");
	emit(AS_Oper(String(assem_string), Temp_TempList(F_FP(), NULL), Temp_TempList(F_SP(), NULL), NULL));
	int stk_size = stack_size(frame);
	if (stk_size) {
		sprintf(assem_string, "sub `d0, %d\n", stk_size);
		emit(AS_Oper(String(assem_string), Temp_TempList(F_SP(), NULL), NULL, NULL));
	}
	//subq	$48, %rsp
	Temp_temp rv = F_RV();
	Temp_temp t;
	for (; sList; sList = sList->tail) {
		t = munchStm(sList->head);
		if (t != F_VOID()) {
			rv = t;
		}
	}
	if (rv != NULL && rv != F_RV()) {
		sprintf(assem_string, "mov `d0, `s0\n");
		emit(AS_Oper(String(assem_string), Temp_TempList(F_RV(), NULL), Temp_TempList(rv, NULL), NULL));
	}
	sprintf(assem_string, "ret\n");
	emit(AS_Oper(String(assem_string),  NULL, NULL, NULL));
	//add ret;

	asList = instrList;
	instrList = last = NULL;
	return asList;
}