#include "semant.h"
#include "env.h"
#include "absyn.h"
#include "errormsg.h"
#include "translate.h"
#include "printtree.h"
#include "canon.h"
#include <stdio.h>
expty expTy(Tr_exp exp, Ty_ty ty) {
	expty e;
	e.exp = exp;
	e.ty = ty;
	return e;
}
static int breakCount = 0;
static void addbreakCount() {
	breakCount++;
}

int typeEq(S_table tenv, Ty_ty a, Ty_ty b) {
	if (a == Ty_Nil() || b == Ty_Nil()) {
		return 1;
	}
	if (actual_ty(tenv, a) == actual_ty(tenv, b)) {
		return 1;
	}
	return 0;
}
F_fragList SEM_transProg(A_exp exp) {
	struct expty et;
	S_table t = E_base_tenv();
	S_table v = E_base_venv();
	Tr_exp breakk = NULL;

	et = transExp(breakk, Tr_outermost(), v, t, exp);
	
	//FILE* out = fopen("tr_exp.txt", "w+");
	Tr_procEntryExit(Tr_outermost(), "letme", et.exp);
	FILE* out = stdout;
	pr_tr(out,  et.exp, 4);

	//printf("\n_________________________________________\n");
	//canon(et.exp);

	//printFrag();

	printf("\n_________________________________________\n");
	printf("\nthis exp return: "); 
	Ty_tyKind(et.ty);
	return Tr_getResult();
}
static void testBreak() {
	if (breakCount <= 0) {
		fck("break not in for or while");
	}
}
U_boolList makeBoolList(A_fieldList f) {
	U_boolList l = NULL;
	for (; f; f = f->tail) {
		l = U_BoolList(f->head->escape, l);
	}
	return l;
}
static void deletebreakCount() {
		breakCount--;
}
bool Ty_tyEqual(Ty_ty lhs, Ty_ty rhs) {
	if (  lhs == rhs || lhs == Ty_Nil() || rhs == Ty_Nil()) {
		return 1;
	}
	return 0;
}
void boom(S_table tenv, S_symbol name, Ty_ty boo) {
	Ty_ty t = S_look(tenv, name);
	if (t == boo) {
		fck("type recursive");
		return;
	} 
	if (t->kind == Ty_name) {
		t = actual_ty(tenv, t);
	}
	if (t != NULL &&t->kind == Ty_record) {
		Ty_fieldList fl = t->u.record;
		for (; fl; fl = fl->tail) {
			Ty_field f = fl->head;
			if (actual_ty(tenv, t) == boo) {
				fck("type recursive");
				return;
			}
			//f = fl->head;
			if (actual_ty(tenv, f->ty) != t) {
				boom(tenv, f->name, boo);
			}
		}
	}
}
void recursive_types_boom(S_table tenv, Ty_ty t, Ty_ty boom) {
		t = actual_ty(tenv, t);
	if (t == boom) {
		fck("type recursive");
		return;
	}
	if (t != NULL &&t->kind == Ty_record) {
		Ty_fieldList fl = t->u.record;
		for (; fl; fl = fl->tail) {
			Ty_field f = fl->head;
			/*if (actual_ty(tenv, t) == boom) {
				fck("type recursive");
				return;
			}*/
			//f = fl->head;
			if (actual_ty(tenv, f->ty) != t) {
				recursive_types_boom(tenv, actual_ty(tenv, f->ty), boom);
			}
		}
	}
	else if (t->kind == Ty_array) {
		Ty_ty arr = t->u.array;
		arr = actual_ty(tenv, arr);
		recursive_types_boom(tenv, arr, t);
	}

}
void recursive_types(S_table tenv, Ty_ty t) {
	//Ty_ty t = S_look(tenv, name);
	t = actual_ty(tenv, t);
	if (t->kind == Ty_record) {
		Ty_fieldList fl = t->u.record;
		for (; fl; fl = fl->tail) {
			Ty_field f = fl->head;
			//f = fl->head;
			if (actual_ty(tenv, f->ty) != t) {
				recursive_types_boom(tenv, f->ty, t);
			}
		}
	}
	else if (t->kind == Ty_array) {
		Ty_ty arr = t->u.array;
		arr = actual_ty(tenv, arr);
		recursive_types_boom(tenv, arr, t);
	}
}
void recursive_type(S_table tenv, S_symbol name) {
	Ty_ty t = S_look(tenv, name);
	t = actual_ty(tenv, t);
	if (t->kind == Ty_record) {
		Ty_fieldList fl = t->u.record;
		for (; fl; fl = fl->tail) {
			Ty_field f = fl->head;
			//f = fl->head;
			if (actual_ty(tenv, f->ty) != t) {
				boom(tenv, f->name, t);
			}
		}
	}
	else if (t->kind == Ty_array) {
		Ty_ty arr = t->u.array;
		arr = actual_ty(tenv, arr);

	}
}
Ty_ty actual_ty(S_table tenv, Ty_ty ty) {
	Ty_ty t;
	if (ty->kind == Ty_name) {
		t = ty;

	/*	if (t->u.name.ty != NULL &&ty->u.name.sym == t->u.name.ty->u.name.sym) {
			fck("type recruse");
			return Ty_Nil();
		}*/
		//t = ty;
		//Ty_print(ty);

		t = S_look(tenv, t->u.name.sym);
		//Ty_print(t);

		while (  t->kind == Ty_name &&  t->u.name.ty != NULL) {
			t = S_look(tenv, t->u.name.ty->u.name.sym);
			Ty_print(t);
			if (t->u.name.sym == ty->u.name.sym) {
				fck("type recursive");
				//exit(0£©
				return Ty_Nil();
			}
		}
		return t;
	}

	else {
		return ty;
	}
}
Ty_tyList makeFormalTyList(S_table tenv, A_fieldList f) {
	if (f) {
		Ty_ty ty = S_look(tenv, f->head->typ);
		//Ty_field field = Ty_Field(f->head->name, ty);
		return Ty_TyList(ty, makeFormalTyList(tenv, f->tail));
	}
	return NULL;
}
Ty_fieldList makefieldList(S_table tenv, A_fieldList f) {
	if (f) {
		Ty_ty ty = S_look(tenv, f->head->typ);
		//Ty_field field = Ty_Field(f->head->name, ty);
		//assert(ty != NULL);
		Ty_field field = Ty_Field(f->head->name, ty);
		S_enter(tenv, f->head->name, ty);
		return  Ty_FieldList(field, makefieldList(tenv, f->tail));
	}
	return NULL;
}


expty transVar(Tr_exp breakk, Tr_level level, S_table venv, S_table tenv, A_var v) {
	/* 
	venv symbol to E_enventry;
	tenv symbol to ty_ty;
	*/
	switch (v->kind)
	{
	case A_simpleVar: {
		E_enventry x = S_look(venv, v->u.simple);
		if (x && x->kind == E_varEntry) {
			return expTy(Tr_simpleVar(x->u.var.access,	 level), actual_ty(tenv,x->u.var.ty));
		}
		EM_error(v->pos, "underfined variable %s", S_name(v->u.simple));
		break;
	}
	case A_subscriptVar: {
		// a[1];
		expty ty = transVar(breakk, level, venv, tenv, v->u.subscript.var);
		if (ty.ty->kind == Ty_array) {
			expty key = transExp(breakk, level, venv, tenv, v->u.subscript.exp);
			if (actual_ty(tenv,key.ty) == Ty_Int()) {
				return  expTy(Tr_subscriptVar(ty.exp, key.exp), actual_ty(tenv,ty.ty->u.array));
			}
		}
		EM_error(v->pos, "underfined variable %s");
		break;
	}
	case A_fieldVar: {
		expty ty = transVar(breakk, level, venv, tenv, v->u.field.var);
		if (ty.ty->kind == Ty_record) {
			Ty_fieldList f = ty.ty->u.record;
			int i = 0;
			while (f && f->head->name != v->u.field.sym) {
				f = f->tail;
				i++;
			}
			if (f != NULL) {
				Tr_exp res = Tr_fieldVar(ty.exp, i);
				return expTy(res, actual_ty(tenv,f->head->ty));
			}
		}
		EM_error(v->pos, "underfined variable %s", S_name(v->u.field.sym));
		break;
	}
	default:
		break;
	}
}
Tr_exp transVarDec(Tr_exp breakk, Tr_level level, S_table venv, S_table tenv, A_dec d) {
	switch (d->kind)
	{
	case A_varDec: {
		Tr_access ac = NULL;
		expty e = transExp(breakk, level, venv, tenv, d->u.var.init);
		if (d->u.var.typ != NULL) {
			Ty_ty ty = S_look(tenv, d->u.var.typ);
			if (actual_ty(tenv, ty) != actual_ty(tenv, e.ty)) {
				Ty_print(actual_ty(tenv, ty));
				//Ty_print(e.ty);
				Ty_print(actual_ty(tenv, e.ty));
				//? equal how
				EM_error(0, "var dec type not equal %s", "");
			}
		}
		ac = Tr_allocLocal(level, d->u.var.escape);
		S_enter(venv, d->u.var.var, E_VarEntry(ac, e.ty));
		return Tr_assign(Tr_simpleVar(ac, level), e.exp);
		break;
	}
	case A_functionDec: {
		A_fundecList list;

		for (list = d->u.function; list; list = list->tail) {
			A_fundec f = list->head;
			Ty_ty resultTy = Ty_Void();
			if (f->result != NULL) {
				resultTy = S_look(tenv, f->result);
				//printf("resty\n");
			}
			Ty_tyList formalTys = makeFormalTyList(tenv, f->params);
			E_enventry funEntry = S_look(venv, f->name);

			S_beginScope(venv);
			{
				A_fieldList l;
				Ty_tyList t;
				Tr_accessList Tr_acl = Tr_formals(funEntry->u.fun.level);
				for (l = f->params, t = formalTys; l; l = l->tail, t = t->tail, Tr_acl = Tr_acl->tail) {
					S_enter(venv, l->head->name, E_VarEntry(Tr_acl->head, t->head));
				}
				expty e = transExp(breakk, funEntry->u.fun.level, venv, tenv, list->head->body);
				if (e.ty != resultTy && e.ty != Ty_Nil() && resultTy != Ty_Nil()) {
					EM_error(0, "function result type not match %s", "");
				}
				Tr_procEntryExit(funEntry->u.fun.level, funEntry->u.fun.name, e.exp);
				printf("ok fundec: %s \n", f->name->name);
			}
			S_endScope(venv);
		}
		break;
	}
	default: {
	}
	}
	return Tr_noExp();
}
void transActualDec(Tr_exp breakk, Tr_level level, S_table venv, S_table tenv, A_dec d) {
	switch (d->kind)
	{
	case A_varDec: {
		break;
	}
	case A_functionDec: {
		break;
	}
	case A_typeDec:{
		A_nametyList n;

		for (n = d->u.type; n; n = n->tail) {
			//Ty_ty ty = S_look(tenv, n->head->name);
			//ty
			//if (ty->kind == Ty_record) {

			//}
			/*Ty_ty ty = transTy(tenv, n->head->ty);
			if (ty->u.name.sym == n->head->name) {
			fck("fuck rec");
			}
			S_enter(tenv, n->head->name, transTy(tenv, n->head->ty));*/
			Ty_ty ty = S_look(tenv, n->head->name);
			Ty_ty rhs = transTy(tenv, n->head->ty);
			if (rhs->kind == Ty_name) {
				ty->u.name.ty = rhs;
			}
			else {
				S_enter(tenv, n->head->name, rhs);
			}
			//actual_ty(tenv, S_look(tenv, n->head->name));
			recursive_types(tenv, S_look(tenv, n->head->name));
		}
		break;
	}
	default:
		break;
	}
}

Tr_exp transDec(Tr_exp breakk, Tr_level level, S_table venv, S_table tenv, A_dec d) {
	switch (d->kind)
	{
	case A_varDec: {
		//var x:= exp
		//Tr_access ac = NULL;
		//expty e = transExp(breakk, level, venv, tenv, d->u.var.init);
		//if (d->u.var.typ != NULL) {
		//	Ty_ty ty = S_look(tenv, d->u.var.typ);
		//	if (actual_ty(tenv, ty) != actual_ty(tenv, e.ty)) {
		//		Ty_print(actual_ty(tenv, ty));
		//		//Ty_print(e.ty);
		//		Ty_print(actual_ty(tenv, e.ty));
		//		//? equal how
		//		EM_error(0, "var dec type not equal %s", "");
		//	}
		//}
		//ac = Tr_allocLocal(level, d->u.var.escape);
		//S_enter(venv, d->u.var.var, E_VarEntry(ac, e.ty));
		//return Tr_assign(Tr_simpleVar(ac, level), e.exp);
		break;
	}
	case A_functionDec: {
		//not list, only one;
		A_fundecList list;
		for (list = d->u.function; list; list = list->tail){
			A_fundec f = list->head;
			Ty_ty resultTy = Ty_Void();
			if (f->result != NULL) {
				resultTy = S_look(tenv, f->result);
			}
			Ty_tyList formalTys = makeFormalTyList(tenv, f->params);
			Temp_label fun_lable = Temp_newlabel();
			Tr_level newlevel = Tr_newLevel(level, fun_lable, makeBoolList(f->params));
			S_enter(venv, f->name, E_FunEntry(newlevel, fun_lable, formalTys, resultTy, S_name(f->name)));
		}
		//for (list = d->u.function; list; list = list->tail) {
		//	A_fundec f = list->head;
		//	Ty_ty resultTy = Ty_Void();
		//	if (f->result != NULL) {
		//		resultTy = S_look(tenv, f->result);
		//		//printf("resty\n");
		//	}
		//	Ty_tyList formalTys = makeFormalTyList(tenv, f->params);
		//	E_enventry funEntry =  S_look(venv, f->name);

		//	S_beginScope(venv);
		//	{
		//		A_fieldList l;
		//		Ty_tyList t;
		//		Tr_accessList Tr_acl = Tr_formals(funEntry->u.fun.level);
		//		for (l = f->params, t = formalTys; l; l = l->tail, t = t->tail, Tr_acl = Tr_acl->tail) {
		//			S_enter(venv, l->head->name, E_VarEntry(Tr_acl->head, t->head));
		//		}
		//		expty e = transExp(breakk, funEntry->u.fun.level, venv, tenv, list->head->body);
		//		if (e.ty != resultTy && e.ty != Ty_Nil() && resultTy != Ty_Nil()) {
		//			EM_error(0, "function result type not match %s", "");
		//		}
		//		Tr_procEntryExit(funEntry->u.fun.level, funEntry->u.fun.name ,e.exp);
		//		printf("ok fundec: %s \n", f->name->name);
		//	}
		//	S_endScope(venv);
		//}
		break;
	}
	case A_typeDec: {
		A_nametyList n;
		for (n = d->u.type; n; n = n->tail) {
			S_enter(tenv, n->head->name, Ty_Name(n->head->name, NULL));
		}
		//for (n = d->u.type; n; n = n->tail) {
		//	//Ty_ty ty = S_look(tenv, n->head->name);
		//	//ty
		//	//if (ty->kind == Ty_record) {

		//	//}
		//	/*Ty_ty ty = transTy(tenv, n->head->ty);
		//	if (ty->u.name.sym == n->head->name) {
		//		fck("fuck rec");
		//	}
		//	S_enter(tenv, n->head->name, transTy(tenv, n->head->ty));*/
		//	Ty_ty ty = S_look(tenv, n->head->name);
		//	Ty_ty rhs = transTy(tenv, n->head->ty);
		//	if (rhs->kind == Ty_name) {
		//		ty->u.name.ty = rhs;
		//	}
		//	else {
		//		S_enter(tenv, n->head->name, rhs); 
		//	}
		//	actual_ty(tenv, S_look(tenv, n->head->name));
		//	recursive_type(tenv, n->head->name);
		//}
		return Tr_noExp();

		break;
	}
	default:
		break;
	}
	return Tr_noExp();

}

expty  transExp( Tr_exp breakk, Tr_level level, S_table venv, S_table tenv, A_exp a) {
	if (a == NULL) {
		return expTy(Tr_noExp(), Ty_Int());
	}
	switch (a->kind)
	{
	case A_varExp:{
		return transVar(breakk, level, venv, tenv, a->u.var);
		break;
	}
	case A_nilExp: {
		return expTy(Tr_nilExp(), Ty_Nil());
		break;
	}
	case A_stringExp: {
		return expTy(Tr_stringExp(a->u.stringg), Ty_String());
		break;
	}
	case A_callExp: {
		E_enventry e = S_look(venv, a->u.call.func);
		if (e == NULL) {
			printf(" not find name %s", S_name(a->u.call.func));
		}
		assert(e != NULL);
		A_expList arg;
		Ty_tyList ty;
		Tr_expList args = NULL;

		for (arg = a->u.call.args, ty = e->u.fun.formals; arg && ty; arg = arg->tail, ty = ty->tail) {
			expty arg1 = transExp(breakk, level, venv, tenv, arg->head);
			args = Tr_ExpList(arg1.exp, args);
			if (arg1.ty !=Ty_Nil() && ty->head != Ty_Nil() && actual_ty(tenv, arg1.ty) != actual_ty(tenv, ty->head)) {
				Ty_tyKind(arg1.ty);
				Ty_tyKind(ty->head);
				fck("call arguments not matched");
			}
		}
		Tr_exp res =  Tr_callExp(e->u.fun.label, e->u.fun.level, level, args);
		if (arg || ty) {
			fck("arguments nums not equal");
		}
		return expTy(res,  actual_ty(tenv, e->u.fun.result));
		break;
	}
	case A_recordExp: {
		Ty_ty ty = S_look(tenv, a->u.record.typ);
		//ty = actual_ty(tenv, ty);
		if (ty->kind == Ty_record) {
			Ty_fieldList tylist = ty->u.record;
			A_efieldList elist = a->u.record.fields;
			Tr_expList recExpList = NULL;
			int attrNum = 0;
			for (; tylist && elist; tylist = tylist->tail, elist = elist->tail) {
				if (tylist->head->name != elist->head->name  ) {
					fck("record name=value; name  not matched");
				}
				expty e = transExp(breakk, level, venv, tenv, elist->head->exp);
				attrNum++;
				recExpList = Tr_ExpList(e.exp, recExpList);
				if (e.ty != actual_ty(tenv, tylist->head->ty) && e.ty != Ty_Nil()) {
					Ty_print(e.ty);
					//Ty_print(S_look(tenv, S_Symbol(String("list")) ));
					//printf("%s, www\n", tylist->head->name->name);
					//Ty_print(tylist->head->ty);
					fck("record name=value; value type  not matched");
				}
			}
			if (elist || tylist) {
				fck("key value nums not equal");
			}
			Tr_exp res = Tr_recordExp(recExpList, attrNum);
			return expTy(res, ty);
		}
		assert(0);
		break;
	}
	case A_seqExp: {
		A_expList exp = a->u.seq;
		if (exp == NULL) {
			return expTy(NULL, Ty_Void());
		}
		//Ty_ty ty;
		expty e;
		Tr_expList list = NULL;
		for (; exp; exp = exp->tail) {
			e = transExp(breakk, level, venv, tenv, exp->head);
			list = Tr_ExpList(e.exp, list);
		}

		return expTy(Tr_seqExp(list), e.ty);
		break;
	}
	case A_assignExp: {
		// lhs = rhs
		expty lhs =  transVar(breakk, level, venv, tenv, a->u.assign.var);
		expty rhs = transExp(breakk, level, venv, tenv, a->u.assign.exp);
		if (lhs.ty != rhs.ty) {
			fck(" assign type not match");
		}
		Tr_exp res = Tr_assign(lhs.exp, rhs.exp);
		return expTy(res, Ty_Void());
		break;
	}
	case A_whileExp: {
		expty test = transExp(breakk, level, venv, tenv, a->u.whilee.test);
		if (test.ty != Ty_Int()) {
			fck(" while test type not int");
		}
		addbreakCount();
		
		Tr_exp done = Tr_breakInit();
		expty body = transExp(done, level, venv, tenv, a->u.whilee.body);
		deletebreakCount();
		if (body.ty != Ty_Void()) {
			fck("while body type should be void");
		}
		Tr_exp res = Tr_whileExp(test.exp, body.exp, done);
		return expTy(res, Ty_Void());
		break;
	}
	case A_ifExp: {
		expty then = expTy(NULL, Ty_Void());
		expty elsee = expTy(NULL, Ty_Void());
		if (a->u.iff.then != NULL) {
			then = transExp(breakk, level, venv, tenv, a->u.iff.then);
		}
		if (a->u.iff.elsee != NULL) {
			elsee = transExp(breakk, level, venv, tenv, a->u.iff.elsee);
		}
		if (!typeEq(tenv, then.ty, elsee.ty)) {
			fck("if type error");
			exit(0);
		}
		if (a->u.iff.test->kind == A_ifExp) {
			A_exp innerIf = a->u.iff.test;
			if (innerIf->u.iff.then->kind == A_intExp && innerIf->u.iff.then->u.intt == 1) {
				expty intterTest = transExp(breakk, level, venv, tenv, innerIf->u.iff.elsee);
				expty newTest = transExp(breakk, level, venv, tenv, A_IfExp(a->pos, innerIf->u.iff.test, a->u.iff.then, a->u.iff.elsee));
				return expTy(Tr_ifExp(intterTest.exp, then.exp, newTest.exp), then.ty);
			}
			else if (innerIf->u.iff.elsee->kind == A_intExp && innerIf->u.iff.elsee->u.intt == 0) {
				expty intterTest = transExp(breakk, level, venv, tenv, innerIf->u.iff.then);
				expty newTest = transExp(breakk, level, venv, tenv, A_IfExp(a->pos, innerIf->u.iff.test, a->u.iff.then, a->u.iff.elsee));
				return expTy(Tr_ifExp(intterTest.exp, newTest.exp, elsee.exp), then.ty);
			}
			assert(0);
		}
		else {
			expty test = transExp(breakk, level, venv, tenv, a->u.iff.test);
			if (test.ty == Ty_Int()) {
				//expty then = expTy(NULL, Ty_Void());
				//expty elsee = expTy(NULL, Ty_Void());
				//if (a->u.iff.then != NULL) {
				//	then = transExp(breakk, level, venv, tenv, a->u.iff.then);
				//}
				//if (a->u.iff.elsee != NULL) {
				//	elsee = transExp(breakk, level, venv, tenv, a->u.iff.elsee);
				//}
				Tr_exp res = Tr_ifExp(test.exp, then.exp, elsee.exp);
				//if (elsee.exp ==)
				if (then.ty == Ty_Void() || elsee.ty == Ty_Void()) {
					return expTy(res, Ty_Void());
				}
				if (then.ty == elsee.ty) {
					return expTy(res, elsee.ty);
				}
				else if (then.ty == Ty_Nil()) {
					return expTy(res, elsee.ty);
				}
				else if (elsee.ty == Ty_Nil()) {
					return expTy(res, then.ty);
				}
				else {
					fck("if return type not equal");
				}
			}
			else {
				fck("if test type != int");
				Ty_print(test.ty);
			}
		}
		
		break;
	}
	case A_forExp: {
		A_dec i_dec = A_VarDec(a->pos, a->u.forr.var, NULL, a->u.forr.lo);
		A_dec limit_dec = A_VarDec(a->pos, S_Symbol(String("high")), NULL, a->u.forr.hi);
		A_dec test_dec = A_VarDec(a->pos, S_Symbol(String("test")), NULL, A_IntExp(a->pos, 1));
		A_decList for_dec = A_DecList(i_dec,
			A_DecList(limit_dec,
				A_DecList(test_dec, NULL)));
		A_exp low_exp = A_VarExp(a->pos, A_SimpleVar(a->pos, a->u.forr.var));
		A_exp test_exp = A_VarExp(a->pos, A_SimpleVar(a->pos, S_Symbol(String("test"))));
		
		A_exp high_Exp = A_VarExp(a->pos, A_SimpleVar(a->pos, S_Symbol(String("high"))));
		A_exp thenexp = A_AssignExp(a->pos, A_SimpleVar(a->pos, a->u.forr.var), A_OpExp(a->pos, A_plusOp, low_exp, A_IntExp(a->pos, 1)));
		A_exp elseexp = A_AssignExp(a->pos, A_SimpleVar(a->pos, S_Symbol(String("test"))), A_IntExp(a->pos, 0));
		A_exp limit_test_exp = A_IfExp(a->pos, A_OpExp(a->pos, A_ltOp, low_exp, high_Exp), thenexp, elseexp);
		A_exp forSeqExp = A_SeqExp(a->pos, A_ExpList(a->u.forr.body, A_ExpList(limit_test_exp, NULL)));
	/*	A_exp test_exp = A_VarExp(a->pos, A_SimpleVar(a->pos, S_Symbol(String("test"))));*/
		A_exp whileExp = A_WhileExp(a->pos, test_exp, forSeqExp);
		A_exp if_Exp = A_IfExp(a->pos, A_OpExp(a->pos, A_leOp, low_exp, high_Exp), whileExp, NULL);
		A_exp let_exp = A_LetExp(a->pos, for_dec, if_Exp);
		//Tr_level newlevel = Tr_newLevel(level, "fordec", NULL);

		return transExp(breakk, level, venv, tenv, let_exp);
		/*expty exp;
		expty lo = transExp(breakk, level, venv, tenv, a->u.forr.hi);
		expty hi = transExp(breakk, level, venv, tenv, a->u.forr.lo);
		if (lo.ty == Ty_Int() && hi.ty == Ty_Int()) {
			S_beginScope(venv);
			S_beginScope(tenv);
			Tr_access ac = Tr_allocLocal(level, TRUE);
			S_enter(venv, a->u.forr.var, E_VarEntry(ac, lo.ty));
			
			addbreakCount();
			exp = transExp(breakk, level, venv, tenv, a->u.forr.body);
			deletebreakCount();
			S_endScope(tenv);
			S_endScope(venv);
			if (exp.ty == Ty_Void()) {
				return expTy(NULL, Ty_Void());
			}
			else {
				fck("for exp type should be void");
			}
		}
		else {
			fck("for high low type error");
		}*/
		break;
	}
	case A_breakExp: {
		testBreak();
		return expTy(Tr_breakExp(breakk), Ty_Void());
		break;
	}
	case A_arrayExp: {
		Ty_ty ty = S_look(tenv, a->u.array.typ);
		expty size = transExp(breakk, level, venv, tenv, a->u.array.size);
		expty init = transExp(breakk, level, venv, tenv, a->u.array.init);
		//ty =
		if (size.ty == Ty_Int()) {
			if (init.ty == ty->u.array) {
				return expTy(Tr_arrayExp(size.exp, init.exp), ty);
			}
			else {
				fck("array init type not match");
			}
		}
		else {
			fck("array size type should be int");
		}
		break;
	}
	case A_intExp: {
		//a->u.intt
		return expTy(Tr_intExp(a->u.intt), Ty_Int());
		break;
	}
	case A_opExp: {
		A_oper oper = a->u.op.oper;
		expty left = transExp(breakk, level, venv, tenv, a->u.op.left);
		expty right = transExp(breakk, level, venv, tenv, a->u.op.right);
		if (!Ty_tyEqual(actual_ty(tenv, left.ty), actual_ty(tenv, right.ty))) {
			fck("op ty should equal");
			Ty_print(left.ty);
			Ty_print(right.ty);
			fck("op ty should equal");
			//return expTy(Tr_noExp(), Ty_Void());
		}
		//if (oper == A_plusOp || oper == A_minusOp || oper == A_timesOp || oper ==  A_divideOp ) {
		//}
		switch (oper)
		{
		case A_plusOp:
		case A_minusOp:
		case A_timesOp:
		case A_divideOp: {
			if (left.ty != Ty_Int()) {
				EM_error(a->u.op.left->pos, "interger required");
			}
			if (right.ty != Ty_Int()) {
				EM_error(a->u.op.right->pos, "interger required");
			}
			return expTy(Tr_binOp(oper, left.exp, right.exp), Ty_Int());

		}
		case A_eqOp:
		case A_neqOp: {
			if (left.ty == Ty_String()) {
				return expTy(Tr_stringCmpExp(oper, left.exp, right.exp), Ty_Int());
			}
			else {
				return expTy(Tr_comOpExp(oper, left.exp, right.exp), Ty_Int());
			}

		}
		case A_ltOp:
		case A_leOp:
		case A_gtOp:
		case A_geOp: {
			return expTy(Tr_comOpExp(oper, left.exp, right.exp), Ty_Int());
		}
		default:
			break;
		}
	
		/*else  {
			return expTy(NULL, Ty_Int());
		}*/
		/*else {
			return expTy(NULL, Ty_Int());
		}*/
		break;
	}
	case A_letExp: {
		expty exp;
		A_decList d;
		S_beginScope(venv);
		S_beginScope(tenv);
		Tr_expList explist = NULL;
		for (d = a->u.let.decs; d; d = d->tail) {
			transDec(breakk, level, venv, tenv, d->head);
			//explist = Tr_ExpList(dec, explist);
		}
		for (d = a->u.let.decs; d; d = d->tail) {
			transActualDec(breakk, level, venv, tenv, d->head);
		}
		for (d = a->u.let.decs; d; d = d->tail) {
			Tr_exp dec = transVarDec(breakk, level, venv, tenv, d->head);
			explist = Tr_ExpList(dec, explist);
		}
		exp = transExp(breakk, level, venv, tenv, a->u.let.body);
		explist = Tr_ExpList(exp.exp, explist);
		Tr_exp letexp = Tr_letExp(explist);
		S_endScope(tenv);
		S_endScope(venv);
		/*if (!trLevelParent(level)) {
			Tr_procEntryExit(level, "letme",  letexp);
		}*/
		return expTy(letexp, exp.ty);
		break;
	}
	default:
		fck("no transexp kind");
		break;
	}
}
Ty_ty transTy(S_table tenv, A_ty d) {
	switch (d->kind)
	{
	case A_nameTy: {
		Ty_ty ty = S_look(tenv, d->u.name);
		if (ty->kind == Ty_name) {
			return ty;
		}
		//while (ty->kind != Ty_name) {
		//	if (ty->u.name.sym == d->u.name) {
		//		fck("type recursive");
		//	}
		//	ty = S_look(tenv, ty->u.name.sym);
		//}
		return ty;
		//if (ty->kind != Ty_record && ty->)
		break;
	}
	case A_recordTy: { 
		A_fieldList l;
		Ty_fieldList fieldList = NULL;

		l = d->u.record;
			//Ty_field f = Ty_Field(l->head->name, ty);
			//fieldList = Ty_FieldList(f, fieldList);
		return Ty_Record(makefieldList(tenv, l));
		break;
	}
	case A_arrayTy: {
		Ty_ty ty = S_look(tenv, d->u.array);
		return Ty_Array(ty);
		break;
	}
	default:
		break;
	}

}

void Ty_tyKind(Ty_ty e) {
	switch (e->kind)
	{
	case Ty_record: {
		printf("record\n");
		break;
	}
	case Ty_nil: {
		printf("nil\n");
		break;
	}
	case Ty_int: {
		printf("int\n");
		break;
	}
	case Ty_string: {
		printf("string\n");
		break;
	}
	case Ty_array: {
		printf("array\n");
		break;
	}
	case Ty_name: {
		printf("name\n");
		break;
	}
	case Ty_void: {
		printf("void\n");
		break;
	}
	default:
		printf("undefined fuck type\n");
		break;
	}

}

