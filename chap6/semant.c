#include  "semant.h"
#include "env.h"
#include "absyn.h"
#include "errormsg.h"
#include "translate.h"
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
void SEM_transProg(A_exp exp) {
	struct expty et;
	S_table t = E_base_tenv();
	S_table v = E_base_venv();
	et = transExp(Tr_outermost(), v, t, exp);
	printf("this exp return: "); 
	Ty_tyKind(et.ty);
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
	if (lhs == rhs || lhs == Ty_Nil() || rhs == Ty_Nil()) {
		return 1;
	}
	return 0;
}
Ty_ty actual_ty(S_table tenv, Ty_ty ty) {
	Ty_ty t;
	if (ty->kind == Ty_name) {
		t = ty;

	/*	if (t->u.name.ty != NULL &&ty->u.name.sym == t->u.name.ty->u.name.sym) {
			fck("type recruse");
			return Ty_Nil();
		}*/
		t = ty;
		//Ty_print(ty);

		t = S_look(tenv, t->u.name.sym);
		//Ty_print(t);

		while (  t->kind == Ty_name &&  t->u.name.ty != NULL) {
			t = S_look(tenv, t->u.name.ty->u.name.sym);
			Ty_print(t);
			if (t->u.name.sym == ty->u.name.sym) {
				fck("type recruse");
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
		Ty_field field = Ty_Field(f->head->name, ty);
		return  Ty_FieldList(field, makefieldList(tenv, f->tail));
	}
	return NULL;
}
// venv symbol to E_enventry;
// tenv symbol to ty_ty;

expty transVar(Tr_level level, S_table venv, S_table tenv, A_var v) {
	switch (v->kind)
	{
	case A_simpleVar: {
		E_enventry x = S_look(venv, v->u.simple);
		if (x && x->kind == E_varEntry) {
			return expTy(NULL, actual_ty(tenv,x->u.var.ty));
		}
		EM_error(v->pos, "underfined variable %s", S_name(v->u.simple));
		break;
	}
	case A_subscriptVar: {
		// a[1];
		expty ty = transVar(level, venv, tenv, v->u.subscript.var);
		if (ty.ty->kind == Ty_array) {
			expty key = transExp(level, venv, tenv, v->u.subscript.exp);
			if (actual_ty(tenv,key.ty) == Ty_Int()) {
				return  expTy(NULL, actual_ty(tenv,ty.ty->u.array));
			}
		}
		EM_error(v->pos, "underfined variable %s");
		break;
	}
	case A_fieldVar: {
		expty ty = transVar(level, venv, tenv, v->u.field.var);
		if (ty.ty->kind == Ty_record) {
			Ty_fieldList f = ty.ty->u.record;
			while (f && f->head->name != v->u.field.sym) {
				f = f->tail;
			}
			if (f != NULL) {
				return expTy(NULL, actual_ty(tenv,f->head->ty));
			}
		}
		EM_error(v->pos, "underfined variable %s", S_name(v->u.field.sym));
		break;
	}
	default:
		break;
	}
}


void  transDec(Tr_level level, S_table venv, S_table tenv, A_dec d) {
	switch (d->kind)
	{
	case A_varDec: {
		//var x:= exp
		Tr_access ac = NULL;
		expty e = transExp(level, venv, tenv, d->u.var.init);
		if (d->u.var.typ != NULL) {
			Ty_ty ty = S_look(tenv, d->u.var.typ);
			if (actual_ty(tenv, ty) != actual_ty(tenv, e.ty)) {
				Ty_print(actual_ty(tenv, ty));
				Ty_print(e.ty);
				Ty_print(actual_ty(tenv, e.ty));

				//? equal how
				EM_error(0, "var dec type not equal %s", "");
			}
		}
		ac = Tr_allocLocal(level, d->u.var.escape);
		S_enter(venv, d->u.var.var, E_VarEntry(ac, e.ty));
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
			S_enter(venv, f->name, E_FunEntry(newlevel, fun_lable, formalTys, resultTy));
		}
		for (list = d->u.function; list; list = list->tail) {
			A_fundec f = list->head;
			Ty_ty resultTy = Ty_Void();
			if (f->result != NULL) {
				resultTy = S_look(tenv, f->result);
				//printf("resty\n");
			}
			Ty_tyList formalTys = makeFormalTyList(tenv, f->params);
			E_enventry funEntry =  S_look(venv, f->name);
			S_beginScope(venv);
			{
				A_fieldList l;
				Ty_tyList t;
				Tr_accessList Tr_acl = Tr_formals(funEntry->u.fun.level);
				for (l = f->params, t = formalTys; l; l = l->tail, t = t->tail, Tr_acl = Tr_acl->tail) {
					S_enter(venv, l->head->name, E_VarEntry(Tr_acl->head, t->head));
				}
				expty e = transExp(funEntry->u.fun.level, venv, tenv, list->head->body);
				if (e.ty != resultTy && e.ty != Ty_Nil() && resultTy != Ty_Nil()) {
					EM_error(0, "function result type not match %s", "");
				}
				printf("ok fundec: %s \n", f->name->name);
			}
			S_endScope(venv);
		}
		break;
	}
	case A_typeDec: {
		A_nametyList n;
		for (n = d->u.type; n; n = n->tail) {
			S_enter(tenv, n->head->name, Ty_Name(n->head->name, NULL));
		}
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
			actual_ty(tenv, S_look(tenv, n->head->name));
		}

		break;
	}
	default:
		break;
	}
}

expty  transExp(Tr_level level, S_table venv, S_table tenv, A_exp a) {
	switch (a->kind)
	{
	case A_varExp:{
		return transVar(level, venv, tenv, a->u.var);
		break;
	}
	case A_nilExp: {
		return expTy(NULL, Ty_Nil());
		break;
	}
	case A_stringExp: {
		return expTy(NULL, Ty_String());
		break;
	}
	case A_callExp: {
		E_enventry e = S_look(venv, a->u.call.func);
		A_expList arg;
		Ty_tyList ty;

		for (arg = a->u.call.args, ty = e->u.fun.formals; arg && ty; arg = arg->tail, ty = ty->tail) {
			expty arg1 = transExp(level, venv, tenv, arg->head);
			if (arg1.ty != ty->head) {
				Ty_tyKind(arg1.ty);
				Ty_tyKind(ty->head);
				fck("call arguments not matched");
			}
		}
		if (arg || ty) {
			fck("arguments nums not equal");
		}
		return expTy(NULL, e->u.fun.result);
		break;
	}
	case A_recordExp: {
		Ty_ty ty = S_look(tenv, a->u.record.typ);
		if (ty->kind == Ty_record) {
			Ty_fieldList tylist = ty->u.record;
			A_efieldList elist = a->u.record.fields;
			for (; tylist && elist; tylist = tylist->tail, elist = elist->tail) {
				if (tylist->head->name != elist->head->name  ) {
					fck("record name=value; name  not matched");
				}
				expty e = transExp(level, venv, tenv, elist->head->exp);
				if (e.ty != actual_ty(tenv, tylist->head->ty) && e.ty != Ty_Nil()) {
					Ty_print(e.ty);
					Ty_print(S_look(tenv, S_Symbol(String("list")) ));
					printf("%s, www\n", tylist->head->name->name);
					Ty_print(tylist->head->ty);
					fck("record name=value; value type  not matched");
				}
			}
			if (elist || tylist) {
				fck("key value nums not equal");
			}
			return expTy(NULL, ty);
		}
		break;
	}
	case A_seqExp: {
		A_expList exp = a->u.seq;
		if (exp == NULL) {
			return expTy(NULL, Ty_Void());
		}
		Ty_ty ty;
		for (; exp; exp = exp->tail) {
			ty = transExp(level, venv, tenv, exp->head).ty;
		}
		return expTy(NULL, ty);
		break;
	}
	case A_assignExp: {
		// lhs = rhs
		expty lhs =  transVar(level, venv, tenv, a->u.assign.var);
		expty rhs = transExp(level, venv, tenv, a->u.assign.exp);
		if (lhs.ty != rhs.ty) {
			fck(" assign type not match");
		}
		return expTy(NULL, Ty_Void());
		break;
	}
	case A_whileExp: {
		expty test = transExp(level, venv, tenv, a->u.whilee.test);
		if (test.ty != Ty_Int()) {
			fck(" while test type not int");
		}
		addbreakCount();
		expty body = transExp(level, venv, tenv, a->u.whilee.body);
		deletebreakCount();
		if (body.ty != Ty_Void()) {
			fck("while body type should be void");
		}
		return expTy(NULL, Ty_Void());
		break;
	}
	case A_ifExp: {
		expty test = transExp(level, venv, tenv, a->u.iff.test);
		if (test.ty == Ty_Int()) {
			expty then = expTy(NULL, Ty_Void());
			expty elsee = expTy(NULL, Ty_Void());
			if (a->u.iff.then != NULL) {
				then = transExp(level, venv, tenv, a->u.iff.then);
			}
			if (a->u.iff.elsee != NULL) {
				elsee = transExp(level, venv, tenv, a->u.iff.elsee);
			}
			if (then.ty == elsee.ty) {
				return expTy(NULL, elsee.ty);
			}
			else if (then.ty == Ty_Nil()) {
				return expTy(NULL, elsee.ty);
			}
			else if (elsee.ty == Ty_Nil()) {
				return expTy(NULL, then.ty);
			}
			else {
				fck("if return type not equal");
			}
		}
		else {
			fck("if test type != int");
			Ty_print(test.ty);
		}
		break;
	}
	case A_forExp: {
		expty exp;
		expty lo = transExp(level, venv, tenv, a->u.forr.hi);
		expty hi = transExp(level, venv, tenv, a->u.forr.lo);
		if (lo.ty == Ty_Int() && hi.ty == Ty_Int()) {
			S_beginScope(venv);
			S_beginScope(tenv);
			Tr_access ac = Tr_allocLocal(level, TRUE);
			S_enter(venv, a->u.forr.var, E_VarEntry(ac, lo.ty));
			
			addbreakCount();
			exp = transExp(level, venv, tenv, a->u.forr.body);
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
		}
		break;
	}
	case A_breakExp: {
		testBreak();
		break;
	}
	case A_arrayExp: {
		Ty_ty ty = S_look(tenv, a->u.array.typ);
		expty size = transExp(level, venv, tenv, a->u.array.size);
		expty init = transExp(level, venv, tenv, a->u.array.init);
		if (size.ty == Ty_Int()) {
			if (init.ty == ty->u.array) {
				return expTy(NULL, ty);
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
		return expTy(NULL, Ty_Int());
		break;
	}
	case A_opExp: {
		A_oper oper = a->u.op.oper;
		expty left = transExp(level, venv, tenv, a->u.op.left);
		expty right = transExp(level, venv, tenv, a->u.op.right);
		if (!Ty_tyEqual(left.ty, right.ty)) {
			fck("op ty should equal");
			Ty_print(left.ty);
			Ty_print(right.ty);
			fck("op ty should equal");

		}
		//if (left.ty != Ty_Int() && left.ty != Ty_String()) {
		//	fck("no op for this type");
		//	Ty_print(left.ty);
		//}
		if (oper == A_plusOp || oper == A_minusOp || oper == A_timesOp || oper ==  A_divideOp ) {
			if (left.ty != Ty_Int()) {
				EM_error(a->u.op.left->pos, "interger required");
			}
			if (right.ty != Ty_Int()) {
				EM_error(a->u.op.right->pos, "interger required");
			}
			return expTy(NULL, Ty_Int());
		}
		else if (oper == A_eqOp || oper == A_neqOp) {
			return expTy(NULL, Ty_Int());
		}
		else {
			return expTy(NULL, Ty_Int());
		}
		break;
	}
	case A_letExp: {
		expty exp;
		A_decList d;
		S_beginScope(venv);
		S_beginScope(tenv);
		for (d = a->u.let.decs; d; d = d->tail) {
			transDec(level, venv, tenv, d->head);
		}
		exp = transExp(level, venv, tenv, a->u.let.body);
		S_endScope(tenv);
		S_endScope(venv);
		return exp;
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