#include "util.h"
#include "prog1.h"
#include "util.h"
#include <stdio.h>
#include <iostream>
#include "interpreter.h"

using namespace std;


int main() {
	A_stm s = prog();
	U_boolList_ b;
	cout << "C is ugly:  " << maxStmArgs(s) << endl;;
	interpStm(s, nullptr);
}