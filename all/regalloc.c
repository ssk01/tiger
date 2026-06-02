#include "regalloc.h"
#include "temp.h"
#include "frame.h"
#include "util.h"
#include "table.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define NUM_REGS 8
#define NUM_ALLOC (NUM_REGS - 2)
#define SPILL_REG1 (NUM_ALLOC)
#define SPILL_REG2 (NUM_ALLOC + 1)

typedef struct {
	int temp_num;
	int start;
	int end;
	int reg;
	int spilled;
	int spill_slot;
} LiveInterval;

static int cmp_by_start(const void *a, const void *b) {
	LiveInterval *ia = *(LiveInterval **)a;
	LiveInterval *ib = *(LiveInterval **)b;
	return ia->start - ib->start;
}

static int count_instructions(AS_instrList ilist) {
	int n = 0;
	for (; ilist; ilist = ilist->tail) n++;
	return n;
}

static void collect_temps(AS_instr instr, Temp_tempList *defs, Temp_tempList *uses) {
	*defs = *uses = NULL;
	switch (instr->kind) {
	case I_OPER:
		*defs = instr->u.OPER.dst;
		*uses = instr->u.OPER.src;
		break;
	case I_MOVE:
		*defs = instr->u.MOVE.dst;
		*uses = instr->u.MOVE.src;
		break;
	case I_LABEL:
		break;
	}
}

static int max_temp_num(AS_instr *instrs, int n) {
	int m = 0;
	for (int i = 0; i < n; i++) {
		Temp_tempList defs, uses;
		collect_temps(instrs[i], &defs, &uses);
		for (Temp_tempList p = defs; p; p = p->tail)
			if (p->head->num > m) m = p->head->num;
		for (Temp_tempList p = uses; p; p = p->tail)
			if (p->head->num > m) m = p->head->num;
	}
	return m;
}

static Temp_temp *build_temp_by_num(int max_num, AS_instr *instrs, int n) {
	Temp_temp *map = checked_malloc((max_num + 1) * sizeof(Temp_temp));
	for (int i = 0; i <= max_num; i++) map[i] = NULL;
	for (int i = 0; i < n; i++) {
		Temp_tempList defs, uses;
		collect_temps(instrs[i], &defs, &uses);
		for (Temp_tempList p = defs; p; p = p->tail) map[p->head->num] = p->head;
		for (Temp_tempList p = uses; p; p = p->tail) map[p->head->num] = p->head;
	}
	return map;
}

static AS_instr make_load(Temp_temp dst, int offset) {
	char buf[200];
	sprintf(buf, "mov `d0, [ebp+%d]\n", offset);
	return AS_Move(String(buf), Temp_TempList(dst, NULL), NULL);
}

static AS_instr make_store(Temp_temp src, int offset) {
	char buf[200];
	sprintf(buf, "mov [ebp+%d], `s0\n", offset);
	return AS_Move(String(buf), NULL, Temp_TempList(src, NULL));
}

static Temp_tempList list_copy(Temp_tempList l) {
	if (!l) return NULL;
	Temp_tempList result = NULL, *tail = &result;
	for (; l; l = l->tail) {
		*tail = Temp_TempList(l->head, NULL);
		tail = &(*tail)->tail;
	}
	return result;
}

typedef struct {
	Temp_temp sp1;
	Temp_temp sp2;
} SpillRegs;

static AS_instr copy_instr_replace(AS_instr old, SpillRegs *sp,
                                   LiveInterval *intervals, int max_num,
                                   int *sp_src_count, int *sp_dst_count) {
	Temp_tempList dst = NULL, src = NULL;

	switch (old->kind) {
	case I_OPER:
		dst = list_copy(old->u.OPER.dst);
		src = list_copy(old->u.OPER.src);
		break;
	case I_MOVE:
		dst = list_copy(old->u.MOVE.dst);
		src = list_copy(old->u.MOVE.src);
		break;
	case I_LABEL:
		return AS_Label(old->u.LABEL.assem, old->u.LABEL.label);
	}

	int sc = 0, dc = 0;
	for (Temp_tempList p = src; p; p = p->tail) {
		int tn = p->head->num;
		if (tn <= max_num && intervals[tn].spilled) sc++;
	}
	for (Temp_tempList p = dst; p; p = p->tail) {
		int tn = p->head->num;
		if (tn <= max_num && intervals[tn].spilled) dc++;
	}
	*sp_src_count = sc;
	*sp_dst_count = dc;

	int si = 0;
	for (Temp_tempList p = src; p; p = p->tail) {
		int tn = p->head->num;
		if (tn <= max_num && intervals[tn].spilled) {
			p->head = (si == 0) ? sp->sp1 : sp->sp2;
			si++;
		}
	}
	int di = 0;
	for (Temp_tempList p = dst; p; p = p->tail) {
		int tn = p->head->num;
		if (tn <= max_num && intervals[tn].spilled) {
			p->head = (di == 0) ? sp->sp1 : sp->sp2;
			di++;
		}
	}

	switch (old->kind) {
	case I_OPER:
		return AS_Oper(old->u.OPER.assem, dst, src, old->u.OPER.jumps);
	case I_MOVE:
		return AS_Move(old->u.MOVE.assem, dst, src);
	default:
		return NULL;
	}
}

AS_instrList RA_linearScan(F_frame frame, AS_instrList ilist) {
	int n = count_instructions(ilist);
	AS_instr *instrs = checked_malloc(n * sizeof(AS_instr));
	{
		int idx = 0;
		for (AS_instrList p = ilist; p; p = p->tail) instrs[idx++] = p->head;
	}

	int max_num = max_temp_num(instrs, n);
	Temp_temp *temp_by_num = build_temp_by_num(max_num, instrs, n);

	LiveInterval *intervals = checked_malloc((max_num + 1) * sizeof(LiveInterval));
	for (int i = 0; i <= max_num; i++) {
		intervals[i].temp_num = i;
		intervals[i].start = 999999;
		intervals[i].end = -1;
		intervals[i].reg = -1;
		intervals[i].spilled = 0;
		intervals[i].spill_slot = 0;
	}

	for (int i = 0; i < n; i++) {
		Temp_tempList defs, uses;
		collect_temps(instrs[i], &defs, &uses);
		for (Temp_tempList p = defs; p; p = p->tail) {
			int tn = p->head->num;
			if (i < intervals[tn].start) intervals[tn].start = i;
			if (i > intervals[tn].end)   intervals[tn].end = i;
		}
		for (Temp_tempList p = uses; p; p = p->tail) {
			int tn = p->head->num;
			if (i < intervals[tn].start) intervals[tn].start = i;
			if (i > intervals[tn].end)   intervals[tn].end = i;
		}
	}

	LiveInterval **sorted = checked_malloc((max_num + 1) * sizeof(LiveInterval *));
	int num_intervals = 0;
	for (int tn = 0; tn <= max_num; tn++) {
		if (intervals[tn].end >= 0) {
			Temp_temp t = temp_by_num[tn];
			string name = t ? Temp_look(Temp_name(), t) : NULL;
			if (!name || name[0] != 'r') continue;
			sorted[num_intervals++] = &intervals[tn];
		}
	}
	qsort(sorted, num_intervals, sizeof(LiveInterval *), cmp_by_start);

	char *phy_reg_names[NUM_REGS];
	int i;
	for (i = 0; i < NUM_REGS; i++) {
		char buf[8];
		sprintf(buf, "R%d", i);
		phy_reg_names[i] = String(buf);
	}

	int free_regs[NUM_ALLOC];
	for (i = 0; i < NUM_ALLOC; i++) free_regs[i] = 1;
	LiveInterval *active[num_intervals + 1];
	int active_len = 0;
	int spill_slot_count = 0;

	for (int j = 0; j < num_intervals; j++) {
		LiveInterval *cur = sorted[j];

		int na = 0;
		for (int k = 0; k < active_len; k++) {
			if (active[k]->end >= cur->start)
				active[na++] = active[k];
			else
				free_regs[active[k]->reg] = 1;
		}
		active_len = na;

		int free_idx = -1;
		for (int r = 0; r < NUM_ALLOC; r++)
			if (free_regs[r]) { free_idx = r; break; }

		if (free_idx >= 0) {
			free_regs[free_idx] = 0;
			cur->reg = free_idx;
			cur->spilled = 0;
		} else {
			int victim = -1, max_end = cur->end;
			for (int k = 0; k < active_len; k++) {
				if (active[k]->end > max_end) {
					max_end = active[k]->end;
					victim = k;
				}
			}
			if (victim >= 0) {
				cur->reg = active[victim]->reg;
				cur->spilled = 0;
				active[victim]->spilled = 1;
				spill_slot_count++;
				active[victim]->spill_slot = -(spill_slot_count * (int)sizeof(intptr_t));
				active[victim] = cur;
			} else {
				cur->spilled = 1;
				spill_slot_count++;
				cur->spill_slot = -(spill_slot_count * (int)sizeof(intptr_t));
			}
		}

		if (!cur->spilled) {
			int pos = 0;
			while (pos < active_len && active[pos]->end < cur->end) pos++;
			for (int k = active_len; k > pos; k--) active[k] = active[k - 1];
			active[pos] = cur;
			active_len++;
		}
	}

	int extra_frame = spill_slot_count * (int)sizeof(intptr_t);

	SpillRegs sp;
	sp.sp1 = Temp_newtemp();
	sp.sp2 = Temp_newtemp();
	{
		char buf[8];
		sprintf(buf, "R%d", SPILL_REG1);
		Temp_enter(Temp_name(), sp.sp1, String(buf));
		sprintf(buf, "R%d", SPILL_REG2);
		Temp_enter(Temp_name(), sp.sp2, String(buf));
	}

	for (int j = 0; j < num_intervals; j++) {
		LiveInterval *cur = sorted[j];
		if (!cur->spilled && temp_by_num[cur->temp_num])
			Temp_enter(Temp_name(), temp_by_num[cur->temp_num],
			           phy_reg_names[cur->reg]);
	}

	AS_instrList result = NULL, *tail = &result;

	for (int i = 0; i < n; i++) {
		AS_instr ins = instrs[i];
		Temp_tempList defs, uses;
		collect_temps(ins, &defs, &uses);

		int sp_src = 0, sp_dst = 0;
		AS_instr new_ins = copy_instr_replace(ins, &sp, intervals, max_num, &sp_src, &sp_dst);

		int si = 0;
		for (Temp_tempList p = uses; p; p = p->tail) {
			int tn = p->head->num;
			if (tn <= max_num && intervals[tn].spilled) {
				int slot = intervals[tn].spill_slot - extra_frame;
				Temp_temp ld_reg = (si == 0) ? sp.sp1 : sp.sp2;
				*tail = AS_InstrList(make_load(ld_reg, slot), NULL);
				tail = &(*tail)->tail;
				si++;
			}
		}

		*tail = AS_InstrList(new_ins, NULL);
		tail = &(*tail)->tail;

		int di = 0;
		for (Temp_tempList p = defs; p; p = p->tail) {
			int tn = p->head->num;
			if (tn <= max_num && intervals[tn].spilled) {
				int slot = intervals[tn].spill_slot - extra_frame;
				Temp_temp st_reg = (di == 0) ? sp.sp1 : sp.sp2;
				*tail = AS_InstrList(make_store(st_reg, slot), NULL);
				tail = &(*tail)->tail;
				di++;
			}
		}
	}

	for (AS_instrList p = result; p; p = p->tail) {
		AS_instr ins = p->head;
		if (ins->kind == I_OPER && strstr(ins->u.OPER.assem, "sub `d0")) {
			char buf[200];
			int orig = 0;
			sscanf(ins->u.OPER.assem, "sub `d0, %d", &orig);
			sprintf(buf, "sub `d0, %d\n", orig + extra_frame);
			ins->u.OPER.assem = String(buf);
			break;
		}
	}

	printf("[RA] virtual regs: %d, spills: %d, extra frame: %d bytes\n",
	       num_intervals, spill_slot_count, extra_frame);

	free(temp_by_num);
	free(sorted);
	free(intervals);
	free(instrs);
	return result;
}
