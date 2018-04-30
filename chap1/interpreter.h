#pragma once
#include "util.h"
#include "slp.h"
struct table;

using tableptr = struct table *;
struct table {
	_string id;
	int value;
	tableptr tail;
};

tableptr update(tableptr t, _string id, int value);
int lookup(tableptr t, _string key);

struct IntAndTable {
	int i;
	tableptr t;
};
int maxExpArgs(A_exp s);
int maxStmArgs(A_stm s);
IntAndTable interpExp(A_exp e, tableptr t);
tableptr interpStm(A_stm s, tableptr t);