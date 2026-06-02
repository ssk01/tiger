#pragma once
#include "assem.h"
#include "frame.h"

typedef struct {
	int num_regs;
	int num_alloc;
	char **reg_names;
} RA_Config;

AS_instrList RA_linearScan_config(F_frame frame, AS_instrList ilist, RA_Config *cfg);
AS_instrList RA_linearScan(F_frame frame, AS_instrList ilist);
