/*
 * parse.c - Parse source file.
 */

#include <stdio.h>
#include "util.h"
#include "symbol.h"
#include "absyn.h"
#include "errormsg.h"
#include "parse.h"
#include "prabsyn.h"
#include "semant.h"
extern int yyparse(void);
extern A_exp absyn_root;

/* parse source file fname;
   return abstract syntax data structure */

A_exp parse(string fname)
{
	EM_reset(fname);
	if (yyparse() == 0) /* parsing worked */
	  //return absyn_root;
	{
		FILE * out = stdout;
		pr_exp(out, absyn_root, 4);
		printf("\n_________________________________________\n");
		SEM_transProg(absyn_root);
		printf("\n_________________________________________\n");
		printFrag();
	}
	else return NULL;
}
int main() {
	parse("fo.tig");
	//parse("let.tig");
	//parse("merge.tig");
	//parse("m.tig");
	//parse("testcases/test16.tig");
	//parse("16.tig");
}