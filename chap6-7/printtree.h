#pragma once
#include "tree.h"
#include "translate.h"
/* function prototype from printtree.c */
void printStmList (FILE *out, T_stmList stmList) ;

void pr_tr(FILE *out, Tr_exp e, int d);