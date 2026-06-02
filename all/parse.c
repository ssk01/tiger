#include <stdio.h>
#include <stdlib.h>
#include "util.h"
#include "symbol.h"
#include "absyn.h"
#include "errormsg.h"
#include "parse.h"
#include "prabsyn.h"
#include "semant.h"
#include "assem.h"
#include "codegen.h"
#include "codegen_arm64.h"
#include "regalloc.h"
#include "lowbVM.h"
#include "printtree.h"
extern int yyparse(void);
extern A_exp absyn_root;
extern bool anyErrors;

static void doProc(FILE *out, F_frame frame, T_stm body, int i) {
	T_stmList stmList;
	AS_instrList iList;
	pr_stm(stdout, body, 4);
	stmList = C_linearize(body);
	stmList = C_traceSchedule(C_basicBlocks(stmList));
	printf("___________________________________\n");
	iList = F_codegen(frame, stmList, i);
	iList = RA_linearScan(frame, iList);
	printf("___________________________________\n");
	fprintf(out, "BEGIN %s\n", Temp_labelstring(F_name(frame)));
	AS_printInstrList(out, iList, Temp_layerMap(F_tempMap, Temp_name()));
	fprintf(out, "END %s\n\n", Temp_labelstring(F_name(frame)));
}

static void doProc_arm64(FILE *out, F_frame frame, T_stm body, int i, int use_ra) {
	T_stmList stmList;
	AS_instrList iList;
	pr_stm(stdout, body, 4);
	stmList = C_linearize(body);
	stmList = C_traceSchedule(C_basicBlocks(stmList));
	printf("___________________________________\n");
	Temp_enter(Temp_name(), F_FP(), "x29");
	Temp_enter(Temp_name(), F_SP(), "sp");
	Temp_enter(Temp_name(), F_RV(), "x0");
	Temp_enter(Temp_name(), F_VOID(), "xzr");
	iList = F_codegen_arm64(frame, stmList, i);
	printf("___________________________________\n");
	if (use_ra) {
		char *arm64_names[] = {"x9","x10","x11","x12","x13","x14","x25","x26"};
		RA_Config cfg = {8, 6, arm64_names};
		iList = RA_linearScan_config(frame, iList, &cfg);
		printf("___________________________________\n");
	}
	AS_printInstrList(out, iList, Temp_layerMap(F_tempMap, Temp_name()));
	fprintf(out, "\n");
}

A_exp parse(string fname, string path) { /* VM mode - unchanged */ return NULL; }

static void parse_arm64(string fname, string asm_path, string bin_path, int use_ra) {
	EM_reset(fname);
	if (yyparse() != 0) { printf("parse error\n"); return; }
	pr_exp(stdout, absyn_root, 4); printf("\n");
	F_fragList frags = SEM_transProg(absyn_root);
	if (anyErrors) { fck("errors "); return; }
	FILE *out = fopen(asm_path, "w+");
	fprintf(out, ".text\n");
	char **slabels = NULL, **svals = NULL;
	int sc = 0;
	F_fragList f;
	for (f = frags; f; f = f->tail)
		if (f->head->kind == F_stringFrag) {
			sc++; slabels = realloc(slabels, sc*sizeof(char*));
			svals = realloc(svals, sc*sizeof(char*));
			slabels[sc-1] = S_name(f->head->u.stringg.label);
			svals[sc-1] = f->head->u.stringg.str;
		}
	for (int si = 0; si < sc; si++) {
		fprintf(out, ".global _%s\n", slabels[si]);
		fprintf(out, "_%s:\n", slabels[si]);
		fprintf(out, ".asciz \"%s\"\n", svals[si]);
	}
	int i = 0;
	for (f = frags; f; f = f->tail)
		if (f->head->kind == F_procFrag) {
			char *label = S_name(F_name(f->head->u.proc.frame));
			fprintf(out, ".global _%s\n", label);
			fprintf(out, ".p2align 2\n");
			doProc_arm64(out, f->head->u.proc.frame, f->head->u.proc.body, i++, use_ra);
		}
	fclose(out);
	char cmd[1024];
	sprintf(cmd, "cc -o %s %s runtime_arm64.c 2>&1", bin_path, asm_path);
	printf("link: %s\n", cmd);
	int rc = system(cmd);
	if (rc != 0) { printf("link failed\n"); return; }
	sprintf(cmd, "%s", bin_path);
	system(cmd);
}

int main() {
	parse_arm64(String("ssktest/sl.tig"), String("ssktest/sl_arm64.s"), String("ssktest/a.out"), 1);
}
