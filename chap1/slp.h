//slp: straight-line program
#pragma once

#include "util.h"
struct A_stm_;
struct A_exp_;
struct A_expList_;
using A_stm  = A_stm_ *;
using A_exp = A_exp_ *;
using A_expList = A_expList_ *;
enum A_binop { A_plus, A_minus, A_times, A_div } ;
enum stmKind{ A_compoundStm, A_assignStm, A_printStm } ;
struct A_stm_ {
	stmKind kind;
	union {
		struct { A_stm stm1; A_stm stm2; } compound;
		struct { _string id; A_exp exp; } assign;
		struct { A_expList exps; } print;
	} u;
};
A_stm A_CompoundStm(A_stm stm1, A_stm stm2);
A_stm A_AssignStm(_string id, A_exp exp);
A_stm A_PrintStm(A_expList exps);
enum expKind { A_idExp, A_numExp, A_opExp, A_eseqExp };
struct A_exp_ {
	expKind kind;
	union {
		_string id;
		int num;
		struct { A_exp left; A_binop oper; A_exp right; } op;
		struct { A_stm stm; A_exp exp; } eseq;
	} u;
};
A_exp A_IdExp(_string id);
A_exp A_NumExp(int num);
A_exp A_OpExp(A_exp left, A_binop oper, A_exp right);
//£¨stm, exp)
A_exp A_EseqExp(A_stm stm, A_exp exp);
enum explistKind { A_pairExpList, A_lastExpList };
struct A_expList_ {
	explistKind kind;
	union {
		struct { A_exp head; A_expList tail; } pair;
		A_exp last;
	} u;
};

A_expList A_PairExpList(A_exp head, A_expList tail);
A_expList A_LastExpList(A_exp last);

