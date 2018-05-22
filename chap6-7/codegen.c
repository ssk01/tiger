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
