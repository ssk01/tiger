/*
 * parse.c - Parse source file.
 */

#include <stdio.h>
#include "util.h"
#include "prabsyn.h"
//#include "symbol.h"
#include "errormsg.h"
extern int yyparse(void);
extern A_exp absyn_root;
extern bptr root;

/* parse source file fname; 
   return abstract syntax data structure */
A_exp parse(string fname) 
{EM_reset(fname);
if (yyparse() == 0) /* parsing worked */
  // return absyn_root;
   //printf("%d, %c, %d", root->lhs->i,root->op, root->rhs->i);
{	
	FILE *f = stdout;
	//FILE *f = fopen("res.txt", "w+");
	pr_exp(f, absyn_root, 4);
 }

 else return NULL;
}
int main(int argc, char **argv) {
 //if (argc!=2) {fprintf(stderr,"usage: a.out filename\n"); exit(1);}
	//parse("19.tig");
	//parse("t.tig");
	//parse("if.txt");
	//parse("merge.tig");
	parse("queens.tig");
	//parse("av.tig");
 return 0;
}