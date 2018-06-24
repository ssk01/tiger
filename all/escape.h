#pragma once
#include "absyn.h"
#include "table.h"

void Esc_findEscape(A_exp exp);
static void tranverseExp(S_table env, int depth, A_exp e);
static void tranverseDec(S_table env, int depth, A_dec e);
static void tranverseVar(S_table env, int depth, A_var e);