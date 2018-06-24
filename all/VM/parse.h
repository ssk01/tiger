#pragma once
#include "vm.h"
void parseString(Vm &v, vector<string>& insLine);
void parseProc(Vm &v, vector<string>& insLine);
void parse(Vm &v, string filename);