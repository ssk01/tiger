#include "codegen.h"
#include "assem.h"
#include "tree.h"
#include "frame.h"
//static F_frame CODEGEN_frame = NULL; // for munchArgs
static AS_instrList instrList = NULL, last = NULL;
//Temp_map F_tempMap = NULL;

static void emit(AS_instr instr)
{
	if (!instrList) instrList = last = AS_InstrList(instr, NULL);
	else last = last->tail = AS_InstrList(instr, NULL);
}
//char *AS_format(char *des, char *format, )
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
			Temp_temp r = Temp_newtemp();
			if (e->u.BINOP.left->kind == T_CONST) {
				T_exp e2 = e->u.BINOP.right;
				int n = e->u.BINOP.left;
				Temp_temp s0 = munchExp(e2);
				sprintf(assem_string, "$s `d0,`s0%s%d\n", op, sign, n);
				emit(AS_Oper(String(assem_string), Temp_TempList(r, NULL), Temp_TempList(s0, NULL), NULL));
				return r;
			}
			else if (e->u.BINOP.right->kind == T_CONST) {
				T_exp e2 = e->u.BINOP.left;
				int n = e->u.BINOP.right;
				Temp_temp s0 = munchExp(e2);
				sprintf(assem_string, "$s `d0,`s0%s%d\n", op, sign, n);
				emit(AS_Oper(String(assem_string), Temp_TempList(r, NULL), Temp_TempList(s0, NULL), NULL));
			}
			else {
				T_exp e1 = e->u.BINOP.left;
				T_exp e2 = e->u.BINOP.right;
				//emit(A)
				//AS_Oper(as
				Temp_temp s0 = munchExp(e1);
				Temp_temp s1 = munchExp(e2);
				sprintf(assem_string, "$s `d0,`s0%s`s1\n", op, sign);
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
					int n = mem->u.BINOP.left;
					Temp_temp s0 = munchExp(e2);
					sprintf(assem_string, "mov	`d0,[`s0+%d]\n", n);
					emit(AS_Move(String(assem_string), Temp_TempList(r, NULL), Temp_TempList(s0, NULL)));
					return r;
				}
				else if (mem->u.BINOP.right->kind == T_CONST) {
					T_exp e2 = mem->u.BINOP.left;
					int n = mem->u.BINOP.right;
					Temp_temp s0 = munchExp(e2);
					sprintf(assem_string, "mov	`d0,[`s0+%d]\n", n);
					emit(AS_Move(String(assem_string), Temp_TempList(r, NULL), Temp_TempList(s0, NULL)));
					return r;
				}
				assert(0);
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
			Temp_enter(F_tempMap, r, Temp_labelstring(e->u.NAME));
			return r;
		}
		case T_CONST: {
			Temp_temp r = Temp_newtemp();
			int n = e->u.CONST;
			sprintf(assem_string, "mov	`d0,%d\n", n);
			emit(AS_Move(String(assem_string),
				Temp_TempList(r, NULL), NULL));
		}
		case T_CALL:
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
		emit(AS_Label(assem_string, stm->u.LABEL));
		break;
	}
	case T_JUMP: {
		Temp_temp r = munchExp(stm->u.JUMP.exp);
		sprintf(assem_string, "jmp	`d0\n");
		emit(AS_Oper(String(assem_string), Temp_TempList(r, NULL), NULL, AS_Targets(stm->u.JUMP.jumps)));
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
			Temp_temp s0 = munchExp(src);
			Temp_temp d0 = munchExp(dst);
			sprintf(assem_string, "move `d0, `s0\n");
			emit(AS_Move(String(assem_string), Temp_TempList(s0, NULL), Temp_TempList(d0, NULL)));
		}
		else if (dst->kind == T_MEM) {
			if (dst->u.MEM->kind == T_BINOP &&
				dst->u.MEM->u.BINOP.op == T_plus &&
				dst->u.MEM->u.BINOP.right->kind == T_CONST) {
				Temp_temp s0 = munchExp(dst->u.MEM->u.BINOP.left);
				Temp_temp s1 = munchExp(src);
				int n = dst->u.MEM->u.BINOP.right->u.CONST;
				sprintf(assem_string, "move `[s0 + %d], `s1\n", n);
				emit(AS_Move(String(assem_string), NULL, Temp_TempList(s0, Temp_TempList(s1, NULL))));
			}
			else if (dst->u.MEM->kind == T_BINOP &&
				dst->u.MEM->u.BINOP.op == T_plus &&
				dst->u.MEM->u.BINOP.left->kind == T_CONST) {
				Temp_temp s0 = munchExp(dst->u.MEM->u.BINOP.right);
				Temp_temp s1 = munchExp(src);
				int n = dst->u.MEM->u.BINOP.left->u.CONST;
				sprintf(assem_string, "move `[s0 + %d], `s1\n", n);
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
				sprintf(assem_string, "mov [s0], `s1\n");
				emit(AS_Move(String(assem_string), NULL, Temp_TempList(s0, Temp_TempList(s1, NULL))));
			}

		}
		else
			assert(0);
		break;

	}
	case T_EXP: {
		munchExp(stm->u.EXP);
		break;
	}
	default:
		assert(0);
		break;
	}
}