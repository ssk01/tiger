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
#include "assem.h"
#include "codegen.h"
#include "printtree.h"
extern int yyparse(void);
extern A_exp absyn_root;
extern bool anyErrors ;

/* parse source file fname;
   return abstract syntax data structure */
static void doProc(FILE *out, F_frame frame, T_stm body)
{
	T_stmList stmList;
	AS_instrList iList;
	stmList = C_linearize(body);
	stmList = C_traceSchedule(C_basicBlocks(stmList));
	printf("___________________________________\n");
	printStmList(stdout, stmList);
	printf("___________________________________\n");
	iList = F_codegen(frame, stmList); /* 9 */

	fprintf(out, "BEGIN %s\n", Temp_labelstring(F_name(frame)));
	AS_printInstrList(out, iList,
		Temp_layerMap(F_tempMap, Temp_name()));
	fprintf(out, "END %s\n\n", Temp_labelstring(F_name(frame)));
}
A_exp parse(string fname)
{
	EM_reset(fname);
	if (yyparse() == 0) /* parsing worked */
	  //return absyn_root;
	{
		FILE * out = stdout;
		//pr_exp(out, absyn_root, 4);
		printf("\n_________________________________________\n");
		F_fragList frags = SEM_transProg(absyn_root);
		if (anyErrors) return NULL; /* don't continue */
		
		/* convert the filename */
		/* Chapter 8, 9, 10, 11 & 12 */
		for (; frags; frags = frags->tail)
			if (frags->head->kind == F_procFrag) {
				printf("proc:%s, %s\n", frags->head->u.proc.name, S_name(F_name(frags->head->u.proc.frame)));
		    doProc(out, frags->head->u.proc.frame, frags->head->u.proc.body);
			}
		    else if (frags->head->kind == F_stringFrag) 
		    fprintf(out, "%s\n", frags->head->u.stringg.str);
		//fclose(out);
		printf("\n_________________________________________\n");

	}
	else return NULL;
}
int main() {
	//parse("print.tig");
	//parse("record.tig");
	//parse("6.tig");
	//parse("fac1.tig");
	//parse("string.tig");
	//parse("array.tig");
	//parse("for.tig");
	//parse("let.tig");
	parse("fun.tig");
	//parse("inner.tig");
	//parse("queens.tig");
	//parse("queens.tig");
	//parse("m.tig");
	//parse("testcases/test16.tig");
	//parse("16.tig");
}