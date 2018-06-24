#pragma once
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <memory.h>
#include <stdlib.h>
#include <assert.h>
#include <set>
using std::cout;
using std::endl;
using std::string;
using std::set;
using std::vector;
using std::map;
class Num {
public:
	enum class Type {
		reg,
		imm,
	};
	int i;
	Type t;
	string reg;
	Num(string reg) :t(Type::reg), i(-1), reg(reg) {
	}
	Num(int i) :t(Type::imm), i(i) {
	}
	~Num() {
		//cout << "doNum" << endl;
	}
};
//int arr[100];
//char *arr;
//int *out;
struct regEnv;
using regEnvPtr = regEnv*;
struct regEnv {
	regEnvPtr parent;
	map<string, int> oldVal;
	~regEnv() {
		//cout << "regEnv deconstructor" << endl;
	}
	regEnv() {
		if (1) {
			//cout << "regEnv constructor" << endl;
		}
	}
};
struct Ins {
public:
	enum class Type {
		MOVCONST2REG,
		MOVREG2REG,
		MOVREG2MEM,
		MOVMEM2REG,
		SUB,
		PUSH,
		MUL,
		ADD,
		POP,
		RET,
		JMP,
		LABEL,
		CALL,
		CMP,
		JE,
		JL,
		JG,
		JGE,
		JNE,
		JLE,
		EXIT,
	};
	Ins() {};
	Ins(Type t) :t(t) {
	}
	//Ins(const Ins &rhs) : t(rhs.t), operNum(rhs.operNum){

	//}
	/*Ins(Ins && rhs): t(std::move(rhs.t)),operNum(std::move(rhs.operNum)) {
	operNum.clear();
	}*/
	//Ins& operator=(const Ins & rhs) {
	//	t = rhs.t;
	//	operNum = rhs.operNum;
	//	return *this;
	//	//t = std::move(rhs.t);
	//	//operNum = std::move(rhs.operNum);
	//	//operNum.clear();
	//}
	//Ins& operator=(Ins && rhs) {
	//	t = std::move(rhs.t);
	//	operNum = std::move(rhs.operNum);
	//	//operNum.clear();
	//	return *this;

	//}
	~Ins() {
		//for (auto num : operNum) {
		//	//delete num;
		//}
		//cout << "Ins destructor" << endl;
	}
	Type t;
	vector<Num *> operNum;
};
//using Type;
//Ins* addLabel(string  label) {
//	auto i = new Ins{ Ins::Type::LABEL };
//	i->u.label = label;
//	return i;
//
//}

class Vm {
private:
	vector<Ins> text;
	int i;
	char *stack;
	int pc;
	map<string, Num*> regs;
	regEnvPtr env;
	map<string, int> labelPos;
	map<string, string> stringData;
	Num *lastNum;
	char *arr;
	char * beg;
public:
	Vm() :i(0), pc(0) {
		//text.resize(400);
		stack = new char[400];
		memset(stack, 0, 400);
		lastNum = nullptr;
		arr = new char[4000];
		beg = arr;
		memset(arr, 0, 4000);
		regs["ebp"] = new Num("ebp");
		regs["esp"] = new Num("esp");
		regs["flag"] = new Num("flag");
		regs["eax"] = new Num("eax");
		regs["flag"]->i = 1;
		regs["ebp"]->i = 400;
		regs["esp"]->i = 400;
		regs["eax"]->i = 1024;
		env = new regEnv();
		env->parent = NULL;
	}
	~Vm() {
		//delete[] stack;
		//delete env;

		cout << "vm deconstructor" << endl;
	}
	void addLabel(string  label) {
		auto ins = Ins{ Ins::Type::LABEL };
		text.push_back((ins));
		labelPos[label] = i;
		i++;
	}
	void addJMP(string  label) {
		auto ins = Ins{ Ins::Type::JMP };
		ins.operNum.push_back(new Num(label));
		text.push_back((ins));
		i++;
	}
	void addCALL(string funname) {
		auto ins = Ins{ Ins::Type::CALL };
		ins.operNum.push_back(new Num(funname));
		text.push_back((ins));
		i++;
	}
	void addCMP(string s1, string s2) {
		auto ins = Ins{ Ins::Type::CMP };
		if (regs.find(s1) == regs.end()) {
			regs[s1] = new Num(s1);
		}
		if (regs.find(s2) == regs.end()) {
			regs[s2] = new Num(s2);
		}
		ins.operNum.push_back(regs[s1]);
		ins.operNum.push_back(regs[s2]);
		text.push_back((ins));
		i++;

	}
	void addJE(string s1) {
		auto ins = Ins{ Ins::Type::JE };
		ins.operNum.push_back(new Num(s1));
		text.push_back((ins));
		i++;
	}
	void addJGE(string s1) {
		auto ins = Ins{ Ins::Type::JGE };
		ins.operNum.push_back(new Num(s1));
		text.push_back((ins));
		i++;
	}
	void addJNE(string s1) {
		auto ins = Ins{ Ins::Type::JNE };
		ins.operNum.push_back(new Num(s1));
		text.push_back((ins));
		i++;
	}
	void addJLE(string s1) {
		auto ins = Ins{ Ins::Type::JLE };
		ins.operNum.push_back(new Num(s1));
		text.push_back((ins));
		i++;
	}
	void addJG(string s1) {
		auto ins = Ins{ Ins::Type::JG};
		ins.operNum.push_back(new Num(s1));
		text.push_back((ins));
		i++;
	}
	void addJL(string s1) {
		auto ins = Ins{ Ins::Type::JL };
		ins.operNum.push_back(new Num(s1));
		text.push_back((ins));
		i++;
	}
	void addRET() {
		auto ins = Ins{ Ins::Type::RET };
		text.push_back((ins));
		i++;
	}
	void addPush(string s1) {
		auto ins = Ins{ Ins::Type::PUSH };
		if (isdigit(s1[0])) {
			ins.operNum.push_back(new Num(atoi(s1.c_str())));
		}
		else {
			if (regs.find(s1) == regs.end()) {
				regs[s1] = new Num(s1);
				if (s1[0] == 'L') {
					regs[s1]->i = reinterpret_cast<int>(stringData[s1].c_str());
				}
			}
			ins.operNum.push_back(regs[s1]);
		}
		text.push_back((ins));
		i++;
	}
	void addMOVCONST2REG(string s1, string  s2) {
		auto ins = Ins{ Ins::Type::MOVCONST2REG };
		if (regs.find(s1) == regs.end()) {
			regs[s1] = new Num(s1);
		}
		ins.operNum.push_back(regs[s1]);
		ins.operNum.push_back(new Num(atoi(s2.c_str())));
		text.push_back((ins));
		i++;
	}
	void addMOVMEM2REG(string  s1, string  s2, string  s3) {
		auto ins = Ins{ Ins::Type::MOVMEM2REG };
		if (regs.find(s1) == regs.end()) {
			regs[s1] = new Num(s1);
		}
		if (regs.find(s2) == regs.end()) {
			regs[s2] = new Num(s2);
		}
		ins.operNum.push_back(regs[s1]);
		ins.operNum.push_back(regs[s2]);
		//assert(atoi(s3.c_str()) > 0);

		ins.operNum.push_back(new Num(atoi(s3.c_str())));

		text.push_back((ins));
		i++;
	}
	void addMOVREG2REG(string  s1, string  s2) {
		auto ins = Ins{ Ins::Type::MOVREG2REG };
		if (regs.find(s1) == regs.end()) {
			regs[s1] = new Num(s1);
		}

		if (regs.find(s2) == regs.end()) {
			if (s2[0] == 'L') {
				regs[s2] = new Num(s2);
				regs[s2]->i = reinterpret_cast<int>(stringData[s2].c_str());
			}
			else {
				regs[s2] = new Num(s2);
			}
		}
		ins.operNum.push_back(regs[s1]);
		ins.operNum.push_back(regs[s2]);
		text.push_back((ins));
		i++;
	}
	void addMOVREG2MEM(string  s1, string  s2, string  s3) {
		auto ins = Ins{ Ins::Type::MOVREG2MEM };
		if (regs.find(s1) == regs.end()) {
			regs[s1] = new Num(s1);
		}

		if (regs.find(s3) == regs.end()) {
			if (s3[0] == 'L') {
				regs[s3] = new Num(s3);
				regs[s3]->i = reinterpret_cast<int>(stringData[s3].c_str());
			}
			else {
				regs[s3] = new Num(s3);
			}
		}
		ins.operNum.push_back(regs[s1]);
		ins.operNum.push_back(new Num(atoi(s2.c_str())));
		ins.operNum.push_back(regs[s3]);
		text.push_back((ins));
		i++;
	}
	void addThree(Ins& ins, string  s1, string  s2, string  s3) {
		if (regs.find(s1) == regs.end()) {
			regs[s1] = new Num(s1);
		}
		ins.operNum.push_back(regs[s1]);
		if (isalpha(s2[0])) {
			if (regs.find(s2) == regs.end()) {
				regs[s2] = new Num(s2);
			}
			ins.operNum.push_back(regs[s2]);
		}
		else {
			ins.operNum.push_back(new Num(atoi(s2.c_str())));
		}
		//ins.operNum.push_back(regs[s2]);
		if (isalpha(s3[0])) {
			if (regs.find(s3) == regs.end()) {
				regs[s3] = new Num(s3);
			}
			ins.operNum.push_back(regs[s3]);
		}
		else {
			ins.operNum.push_back(new Num(atoi(s3.c_str())));
		}
	}
	void addMul(string  s1, string  s2, string  s3) {
		auto ins = Ins{ Ins::Type::MUL };
		addThree(ins, s1, s2, s3);
		text.push_back((ins));
		i++;
	}
	void addSub(string  s1, string  s2, string  s3) {
		auto ins = Ins{ Ins::Type::SUB };
		addThree(ins, s1, s2, s3);
		text.push_back((ins));
		i++;
	}
	void addAdd(string  s1, string  s2, string  s3) {
		auto ins = Ins{ Ins::Type::ADD };
		addThree(ins, s1, s2, s3);
		text.push_back((ins));
		i++;
	}
	void Run() {
		//regs["eax"]->i = -1;
		int dead = 0;
		while (pc < text.size()) {
			dead++;
			/*if (dead > 5000) {
				cout << "dead  "<<pc << endl;
				exit(0);
			}*/
			auto ins = text[pc];
			switch (ins.t)
			{
			case Ins::Type::LABEL: {
				break;
			}
			case Ins::Type::MUL: {
				auto reg1 = ins.operNum[0];
				auto reg2 = ins.operNum[1];
				auto reg3 = ins.operNum[2];
				reg1->i = reg2->i * reg3->i;
				lastNum = reg3;
				break;

			}
			case Ins::Type::MOVCONST2REG: {
				auto reg1 = ins.operNum[0];
				auto reg2 = ins.operNum[1];
				reg1->i = reg2->i;
				lastNum = reg1;
				/*cout << regs[reg1->reg]->i << endl;*/
				break;

			}
			case Ins::Type::MOVREG2REG: {
				auto reg1 = ins.operNum[0];
				auto reg2 = ins.operNum[1];
				reg1->i = reg2->i;
				lastNum = reg1;

				break;

			}
			case Ins::Type::MOVREG2MEM: {
				auto reg1 = ins.operNum[0];
				auto reg2 = ins.operNum[1];
				auto reg3 = ins.operNum[2];
				if (reg1->i + reg2->i >= (int)beg) {
					*(reinterpret_cast<int *>(reg1->i + reg2->i)) = reg3->i;
				}
				else {
					*(reinterpret_cast<int *>(stack + reg1->i + reg2->i)) = reg3->i;
				}
				break;
			}
			case Ins::Type::MOVMEM2REG: {
				auto reg1 = ins.operNum[0];
				auto reg2 = ins.operNum[1];
				auto reg3 = ins.operNum[2];

				if (reg2->i + reg3->i >= (int)beg) {
					reg1->i = *(reinterpret_cast<int *>(reg2->i + reg3->i));
				}
				else {
					reg1->i = *(reinterpret_cast<int *>(stack + reg2->i + reg3->i));

				}
				lastNum = reg1;

				/*	if (reg1->reg == "r121") {
				cout << (char*)reg1->i << endl;
				}
				if (reg1->reg == "eax") {}*/
				//cout << regs["esp"]->i << endl;
				//cout << regs["ebp"]->i << endl;
				break;
			}
			case Ins::Type::SUB: {
				auto reg1 = ins.operNum[0];
				auto reg2 = ins.operNum[1];
				auto reg3 = ins.operNum[2];
				reg1->i = reg2->i - reg3->i;
				lastNum = reg1;

				break;
			}
			case Ins::Type::PUSH: {
				regs["esp"]->i -= 4;
				*(int *)(stack + regs["esp"]->i) = ins.operNum[0]->i;
				break;

			}
			case Ins::Type::JE: {
				if (regs["flag"]->i == 1) {
					pc = labelPos[ins.operNum[0]->reg];
				}
				break;
			}

			case Ins::Type::JLE: {
				if (regs["flag"]->i == 3 || regs["flag"]->i == 1) {
					pc = labelPos[ins.operNum[0]->reg];
				}
				break;
			}
			case Ins::Type::JL: {
				if (regs["flag"]->i == 3) {
					pc = labelPos[ins.operNum[0]->reg];
				}
				break;
			}
			case Ins::Type::JGE: {
				if (regs["flag"]->i == 2 || regs["flag"]->i == 1) {
					pc = labelPos[ins.operNum[0]->reg];
				}
				break;
			}
			case Ins::Type::JNE: {
				if (regs["flag"]->i != 1) {
					pc = labelPos[ins.operNum[0]->reg];
				}
				break;
			}
			case Ins::Type::JG: {
				if (regs["flag"]->i == 2) {
					pc = labelPos[ins.operNum[0]->reg];
				}
				break;
			}
			case Ins::Type::CMP: {
				auto reg1 = ins.operNum[0];
				//cout << regs["r102"]->i << endl;
				auto reg2 = ins.operNum[1];
				if (reg1->i == reg2->i) {
					regs["flag"]->i = 1;
				}
				else if (reg1->i > reg2->i) {
					regs["flag"]->i = 2;
				}
				else if (reg1->i < reg2->i) {
					regs["flag"]->i = 3;
				}
				break;
			}
			case Ins::Type::ADD: {
				auto reg1 = ins.operNum[0];
				auto reg2 = ins.operNum[1];
				auto reg3 = ins.operNum[2];
				reg1->i = reg2->i + reg3->i;
				lastNum = reg1;

				break;
			}
			case Ins::Type::POP: {
				break;
			}
			case Ins::Type::RET: {
				/*	cout << "ret value" << regs["eax"]->i << endl;*/
				pc = *(reinterpret_cast<int *>(stack + regs["ebp"]->i + 4));
				if (lastNum != nullptr) {
					regs["eax"]->i = lastNum->i;
					lastNum = nullptr;
				}
				if (pc == -1) {
					//env = env->parent;
					//lastNum = reg1;

					cout << "end result: " << regs["eax"]->i << endl;

					return;
				}
				regs["esp"]->i = regs["ebp"]->i;
				regs["ebp"]->i = *(reinterpret_cast<int *>(stack + regs["ebp"]->i));
				set<string> special{ "esp","ebp","eax" };
				for (auto reg : env->oldVal) {
					if (special.find(reg.first) == special.end()) {
						regs[reg.first]->i = reg.second;
					}
				}
				auto p = env;
				env = env->parent;
				delete p;
				break;

			}
			case Ins::Type::JMP: {
				auto label = ins.operNum[0];
				pc = labelPos[label->reg];
				break;
			}
			case Ins::Type::CALL: {
				auto label = ins.operNum[0];
				if (label->reg == "printInt") {
					auto arg = *(int *)(stack + regs["esp"]->i + 4);
					//cout << regs["eax"]->i << endl;
					//printf("addr %x\n", (int *)(stack + regs["esp"]->i + 4));
					cout << "Print int: " << arg << endl;
				}
				else if (label->reg == "print") {
					auto arg = *(int *)(stack + regs["esp"]->i + 4);
					string strs{ (char *)arg };
					if (strs.substr(0,7) == "newline") {
						cout <<"\n"<< endl;
					}
					else {
						cout << strs << " ";
						//cout << "Print str: " << strs << endl;
					}
				}
				else if (label->reg == "ord") {
					auto arg = *(int *)(stack + regs["esp"]->i + 4);
					string strs{ (char *)arg };
					cout << "ord "<<strs[0] << endl;
					regs["eax"]->i = int(strs[0]);
				}
				else if (label->reg == "getchar") {
					static char input[20] = "a1a7a8aabaa4a5a6";
					static int index = 0;
					string c{ input[index] };
					//char *c = 
					cout << "index " << index << " "<<c<< endl;
					stringData[c] = c;
					index++;
					regs["eax"]->i = reinterpret_cast<int >(stringData[c].c_str());
					cout << "regs[eax]->i" << regs["eax"]->i << endl;
				}
				else if (label->reg == "malloc") {
					auto size = *(int *)(stack + regs["esp"]->i + 0);
					if (size == 0) {
						regs["eax"]->i = 0xff;
					}
					else {
						regs["eax"]->i = reinterpret_cast<int >(arr);
						cout << "malloc addr :" << regs["eax"]->i << endl;
						arr += size;

					}
				}
				else if (label->reg == "stringEqual")
				{
					auto str1 = *(int *)(stack + regs["esp"]->i + 0);
					auto str2 = *(int *)(stack + regs["esp"]->i + 4);
					if (string((char*)str1) == string((char*)str2)) {
						regs["eax"]->i = 1;
					}
					else {
						regs["eax"]->i = 0;
					}
				}
				else if (label->reg == "initArray") {
					// extern call no stack link
					auto num = *(int *)(stack + regs["esp"]->i + 0);
					auto init = *(int *)(stack + regs["esp"]->i + 4);
					cout << "int" << init << endl;
					cout << "int" << num << endl;
					/*	arr[3] = 4;
					arr[4] = 4;
					arr[2] = 4;*/
					for (auto i = 0; i < num; i++) {
						*((int*)arr + i) = init;
					}
					regs["eax"]->i = reinterpret_cast<int >(arr);
					arr = arr + 4 * num;
					break;

					//auto val = new int[num];
					//auto val = (int *)malloc(sizeof(int)*num);
					//memset(val,  init, num);
					//out = val;
					//printf("%x...\n", val);

					//printf("aaa %x\n", val);
					//for (int i = 0; i < init; i++) {
					//	val[i] = init;
					//}
					////cout << val[5] << endl;
					//int add = reinterpret_cast<int>( val);
					////printf("aaa %x\n", add);
					//regs["eax"]->i = add;
					//cout << "init array" << regs["eax"]->i<<endl;
				}
				else {
					assert(label->reg[0] == 'L');
					regs["esp"]->i -= 4;
					int a = regs["esp"]->i;
					*(int *)(stack + regs["esp"]->i) = pc;
					pc = labelPos[label->reg];
					auto oldEnv = new regEnv();
					oldEnv->parent = env;
					for (auto reg : regs) {
						oldEnv->oldVal[reg.first] = reg.second->i;
					}
					env = oldEnv;
				}
				break;

			}
			default:
				cout << "ins->t not eval" << (int)ins.t << endl;
				assert(0);
				break;
			}
			pc++;

		}
	}
	void addString(string label, string str) {
		stringData[label] = str;
		cout << (int*)stringData[label].c_str() << endl;
	}
};