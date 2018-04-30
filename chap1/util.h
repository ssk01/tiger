#pragma once

#include <assert.h>
using _string = char *;
//typedef char bool;

#define TRUE 1
#define FALSE 0

void *checked_malloc(int);
_string String(char *);
struct U_boolList_;
using U_boolList=  U_boolList_ *;
struct U_boolList_ {
	bool head; 
	U_boolList tail;
};
U_boolList U_BoolList(bool head, U_boolList tail);
