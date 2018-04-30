#include "interpreter.h"
#include "slp.h"
#include <iostream>
using namespace std;
tableptr update(tableptr t, _string id, int value) {
	auto newt = new table;
	newt->id = id;
	newt->value = value;
	newt->tail = t;
	return newt;
}
int lookup(tableptr t, _string key) {
	while (t != nullptr) {
		if (t->id == key) {
			return t->value;
		}
		t = t->tail;
	}
	assert(0);
	return -1;
}


int max(int a, int b) {
	return a > b ? a : b;
}

tableptr interpStm(A_stm s, tableptr t) {
	if (s->kind == A_compoundStm) {
		t = interpStm(s->u.compound.stm1, t);
		t = interpStm(s->u.compound.stm2, t);
		return t;
	}
	else if (s->kind == A_assignStm) {
		auto intTable = interpExp(s->u.assign.exp, t);
		//bug?
		t = intTable.t;
		return update(t, s->u.assign.id, intTable.i);
	}
	else if (s->kind == A_printStm) {
		A_expList list = s->u.print.exps;
		auto cur = t;
		while (list->kind != A_lastExpList) {
			auto intTable = interpExp(list->u.pair.head, cur);
			cout << "print: " << intTable.i << endl;
			cur = intTable.t;
			list = list->u.pair.tail;
		}
		auto intTable = interpExp(list->u.last, cur);
		cout << "print: " << intTable.i << endl;
		return intTable.t;
	}
}

IntAndTable interpExp(A_exp s, tableptr t) {
	if (s->kind == A_numExp) {
		return{ s->u.num, t };
	}
	else if (s->kind == A_idExp) {
		auto i = lookup(t, s->u.id);
		return{ i, t };
	}
	else if (s->kind == A_opExp) {
		auto intTable = interpExp(s->u.op.left, t);
		auto lhs = intTable.i;
		intTable = interpExp(s->u.op.right, intTable.t);
		auto rhs = intTable.i;
		auto i = -1;
		switch (s->u.op.oper)
		{
		case A_minus:
			i = lhs - rhs;
			break;
		case A_plus:
			i = lhs + rhs;
			break;
		case A_times:
			i = lhs * rhs;
			break;
		case A_div:
			i = lhs / rhs;
		default:
			assert(0);
			break;
		}
		return{ i, intTable.t };
	}
	else if (s->kind == A_eseqExp) {
		t = interpStm(s->u.eseq.stm, t);
		return interpExp(s->u.eseq.exp, t);
	}
}

//max print args for A_exp
int maxExpArgs(A_exp s) {
	if (s->kind == A_idExp || s->kind == A_numExp) {
		return 0;
	}
	else if (s->kind == A_opExp) {
		return max(maxExpArgs(s->u.op.left), maxExpArgs(s->u.op.right));
	}
	else if (s->kind == A_eseqExp) {
		return max(maxStmArgs(s->u.eseq.stm), maxExpArgs(s->u.eseq.exp));
	}
}

//max print args for A_stm;
int maxStmArgs(A_stm s) {
	if (s->kind == A_compoundStm) {
		int a = maxStmArgs(s->u.compound.stm1);
		int b = maxStmArgs(s->u.compound.stm2);
		return a > b ? a : b;
	}
	else if (s->kind == A_assignStm) {
		return maxExpArgs(s->u.assign.exp);
	}
	else if (s->kind == A_printStm) {
		A_expList list = s->u.print.exps;
		int i = 0;
		int maxs = 0;
		while (list->kind != A_lastExpList) {
			i++;
			maxs = max(maxs, maxExpArgs(list->u.pair.head));
			list = list->u.pair.tail;
		}
		i++;
		maxs = max(maxs, maxExpArgs(list->u.last));
		return max(maxs, i);
	}
}