#pragma once
#include "tree.h"
/* function prototype from printtree.c */
void printStmList (FILE *out, T_stmList stmList) ;
void pr_tree_exp(FILE *out, T_exp exp, int d);
void pr_stm(FILE *out, T_stm stm, int d);
//void pr_tr(FILE *out, T_exp e, int d);
