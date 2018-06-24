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
#include "lowbVM.h"
#include "printtree.h"
extern int yyparse(void);
extern A_exp absyn_root;
extern bool anyErrors ;

/* parse source file fname;
   return abstract syntax data structure */
static void doProc(FILE *out, F_frame frame, T_stm body, int i)
{
	T_stmList stmList;
	AS_instrList iList;
	pr_stm(stdout, body, 4);

	stmList = C_linearize(body);
	stmList = C_traceSchedule(C_basicBlocks(stmList));
	printf("___________________________________\n");
	//printStmList(stdout, stmList);
	iList = F_codegen(frame, stmList, i); /* 9 */
	printf("___________________________________\n");
	//out = fopen("fac_1.txt", "a+");
	//out = stdout;
	fprintf(out, "BEGIN %s\n", Temp_labelstring(F_name(frame)));
	AS_printInstrList(out, iList,
		Temp_layerMap(F_tempMap, Temp_name()));
	fprintf(out, "END %s\n\n", Temp_labelstring(F_name(frame)));
	////fprintf(out, "______________ \n\n");
}
A_exp parse(string fname, string path)
{
	EM_reset(fname);
	if (yyparse() == 0) /* parsing worked */
	  //return absyn_root;
	{
		FILE * out = stdout;
		out = fopen(path, "w+");
		//out = stdout;
		pr_exp(stdout, absyn_root, 4);
		printf("\n_________________________________________\n");
		F_fragList frags = SEM_transProg(absyn_root);
		if (anyErrors) {
			fck("errors ");
			return NULL; /* don't continue */
		}
		/* convert the filename */
		/* Chapter 8, 9, 10, 11 & 12 */
		int i = 0;
		fprintf(out, "string: \n\n");
		for (; frags; frags = frags->tail)
			if (frags->head->kind == F_procFrag) {
				fprintf(out ,"proc:     %s, %s\n\n", frags->head->u.proc.name, S_name(F_name(frags->head->u.proc.frame)));
		    doProc(out, frags->head->u.proc.frame, frags->head->u.proc.body, i++);
			}
			else if (frags->head->kind == F_stringFrag) {
				fprintf(out, "%s: %s\n", S_name(frags->head->u.stringg.label),frags->head->u.stringg.str);
			}
		//fclose(out);
		printf("\n_________________________________________\n");

	}
	else return NULL;
}
int main() {
	//parse("print.tig");

	//parse("record.tig");
	//parse("ssktest/while.tig");
	//parse("while.test");
	//parse("while.test");
	
	parse(String("ssktest/king.tig"), String("ssktest/king.txt"));
	//parse(String("ssktest/slice.tig"), String("ssktest/slice.txt"));
	//parse(String("ssktest/iff.tig"), String("ssktest/iff.txt"));

	//parse(String("ssktest/mul.tig"), String("ssktest/mul.txt"));
	//parse("ssktest/king.tig", String("ssktest/king.txt"));
	//parse("ssktest/test12.tig", String("ssktest/test12.txt"));
	//parse("fac.tig");
	//parse("string.tig");
	//parse("array.tig");
	//parse("for.tig");
	//parse("let.tig");
	//parse("fun.tig");
	//parse("2.tig");
	//testVM();
	//parse("inner.tig");
	//parse("queens.tig");
	//parse("queens.tig");
	//parse("m.tig");
	//parse("testcases/test16.tig");
	//parse("16.tig");
}