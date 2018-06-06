#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <assert.h>
#include <set>
using namespace std;
class Num {
public:
	enum class Type {
		reg,
		imm,
	};
	int i;
	Type t;
	string reg;
	Num(string reg):t(Type::reg),i(-1),reg(reg) {
	}
	Num(int i) :t(Type::imm), i(i) {
	}
};
struct regEnv;
using regEnvPtr = regEnv*;
struct regEnv {
	regEnvPtr parent;
	map<string, int> oldVal;
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
		EXIT,
	};
	Ins() {};
	Ins(Type t) :t(t) {
	}
	Type t;
	vector<Num *> operNum;
};
//using Type;
//Ins* addLabel(string& label) {
//	auto i = new Ins{ Ins::Type::LABEL };
//	i->u.label = label;
//	return i;
//
//}
 
class Vm {
public:
	Vm():i(0),pc(0) {
		text.resize(400);
		stack = new char[400];
		memset(stack, 0, 400);
		regs["ebp"] = new Num("ebp");
		regs["esp"] = new Num("esp");
		regs["flag"] = new Num("flag");
		regs["flag"]->i = 1;
		regs["ebp"]->i = 400;
		regs["esp"]->i = 400;
		env = new regEnv;
		env->parent = NULL;
	}
	void addLabel(string& label) {
		auto ins = Ins{ Ins::Type::LABEL };
		text[i] = ins;
		labelPos[label] = i;
		i++;
	}
	void addJMP(string & label) {
		auto ins = Ins{ Ins::Type::JMP };
		ins.operNum.push_back(new Num(label));
		text[i] = ins;
		i++;
	}
	void addCALL(string& funname) {
		auto ins = Ins{ Ins::Type::CALL };
		ins.operNum.push_back(new Num(funname));
		text[i] = ins;
		i++;
	}
	void addCMP(string &s1, string &s2) {
		auto ins = Ins{ Ins::Type::CMP };
		if (regs.find(s1) == regs.end()) {
			regs[s1] = new Num(s1);
		}
		if (regs.find(s2) == regs.end()) {
			regs[s2] = new Num(s2);
		}
		ins.operNum.push_back(regs[s1]);
		ins.operNum.push_back(regs[s2]);
		text[i] = ins;
		i++;

	}
	void addJE(string &s1) {
		auto ins = Ins{ Ins::Type::JE };
		ins.operNum.push_back(new Num(s1));
		text[i] = ins;
		i++;
	}
	void addRET() {
		auto ins = Ins{ Ins::Type::RET };
		text[i] = ins;
		i++;
	}
	void addPush(string &s1) {
		auto ins = Ins{ Ins::Type::PUSH };
		if (isdigit(s1[0])) {
			ins.operNum.push_back(new Num(atoi(s1.c_str())));
		}
		else {
			if (regs.find(s1) == regs.end()) {
				regs[s1] = new Num(s1);
			}
			ins.operNum.push_back(regs[s1]);
		}
		text[i] = ins;
		i++;
	}
	void addMOVCONST2REG(string &s1, string& s2) {
		auto ins = Ins{ Ins::Type::MOVCONST2REG };
		if (regs.find(s1) == regs.end()) {
			regs[s1] = new Num(s1);
		}
		ins.operNum.push_back(regs[s1]);
		ins.operNum.push_back(new Num(atoi(s2.c_str())));
		text[i] = ins;
		i++;
	}
	void addMOVMEM2REG(string& s1, string& s2, string& s3) {
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
		if (s1 == "r120") {
			cout << "sf        " << s3 << endl;
		}
		ins.operNum.push_back(new Num(atoi(s3.c_str())));

		text[i] = ins;
		i++;
	}
	void addMOVREG2REG(string& s1, string& s2) {
		auto ins = Ins{ Ins::Type::MOVREG2REG };
		if (regs.find(s1) == regs.end()) {
			regs[s1] = new Num(s1);
		}
		if (regs.find(s2) == regs.end()) {
			regs[s2] = new Num(s2);
		}
		ins.operNum.push_back(regs[s1]);
		ins.operNum.push_back(regs[s2]);
		text[i] = ins;
		i++;
	}
	void addMOVREG2MEM(string& s1, string& s2, string& s3) {
		auto ins = Ins{ Ins::Type::MOVREG2MEM };
		if (regs.find(s1) == regs.end()) {
			regs[s1] = new Num(s1);
		}
		if (regs.find(s3) == regs.end()) {
			regs[s3] = new Num(s3);
		}
		ins.operNum.push_back(regs[s1]);
		ins.operNum.push_back(new Num(atoi(s2.c_str())));
		ins.operNum.push_back(regs[s3]);	
		text[i] = ins;
		i++;
	}
	void addThree( Ins& ins, string& s1, string& s2, string& s3) {
		if (regs.find(s1) == regs.end()) {
			regs[s1] = new Num(s1);
		}
		if (regs.find(s2) == regs.end()) {
			regs[s2] = new Num(s2);
		}
		ins.operNum.push_back(regs[s1]);
		ins.operNum.push_back(regs[s2]);
		if (s3[0] == 'r') {
			if (regs.find(s3) == regs.end()) {
				regs[s3] = new Num(s3);
			}
			ins.operNum.push_back(regs[s3]);
		}
		else {
			ins.operNum.push_back(new Num(atoi(s3.c_str())));
		}
	}
	void addMul(string& s1, string& s2, string& s3) {
		auto ins = Ins{ Ins::Type::MUL };
		addThree(ins, s1, s2, s3);
		text[i] = ins;
		i++;
	}
	void addSub(string& s1, string& s2, string& s3) {
		auto ins = Ins{ Ins::Type::SUB };
		addThree(ins, s1, s2, s3);
		text[i] = ins;
		i++;
	}
	void addAdd(string& s1, string& s2, string& s3) {
		auto ins = Ins{ Ins::Type::ADD };
		addThree(ins, s1, s2, s3);
		text[i] = ins;
		i++;
	}
	void Run() {
		//regs["eax"]->i = -1;
		while (pc < text.size()) {
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
				break;

			}
			case Ins::Type::MOVCONST2REG: {
				auto reg1 = ins.operNum[0];
				auto reg2 = ins.operNum[1];
				reg1->i = reg2->i;
				/*cout << regs[reg1->reg]->i << endl;*/
				break;

			}
			case Ins::Type::MOVREG2REG: {
				auto reg1 = ins.operNum[0];
				auto reg2 = ins.operNum[1];
				reg1->i = reg2->i;
				break;

			}
			case Ins::Type::MOVREG2MEM: {
				auto reg1 = ins.operNum[0];
				auto reg2 = ins.operNum[1];
				auto reg3 = ins.operNum[2];
				*(reinterpret_cast<int *>(stack + reg1->i + reg2->i)) = reg3->i;
				break;
			}
			case Ins::Type::MOVMEM2REG: {
				auto reg1 = ins.operNum[0];
				auto reg2 = ins.operNum[1];
				auto reg3 = ins.operNum[2];
				//cout << regs["esp"]->i << endl;
				//cout << regs["ebp"]->i << endl;
				reg1->i = *(reinterpret_cast<int *>(stack + reg2->i + reg3->i));
				break;
			}
			case Ins::Type::SUB: {
				auto reg1 = ins.operNum[0];
				auto reg2 = ins.operNum[1];
				auto reg3 = ins.operNum[2];
				reg1->i = reg2->i - reg3->i;
				break;
			}
			case Ins::Type::PUSH: {
				regs["esp"]->i -= 4;
				*(int *)(stack + regs["esp"]->i) = ins.operNum[0]->i;
				break;

			}
			case Ins::Type::JE: {
				if (regs["flag"]->i) {
					pc = labelPos[ins.operNum[0]->reg];
				}
				/*cout <<"ebps " <<regs["ebp"]->i << endl;*/
				break;
			}
			case Ins::Type::CMP: {
				auto reg1 = ins.operNum[0];
				//cout << regs["r102"]->i << endl;
				auto reg2 = ins.operNum[1];
				if (reg1->i == reg2->i) {
					regs["flag"]->i = 1;
				}
				else {
					regs["flag"]->i = 0;
				}
				break;
			}
			case Ins::Type::ADD: {
				auto reg1 = ins.operNum[0];
				auto reg2 = ins.operNum[1];
				auto reg3 = ins.operNum[2];
				reg1->i = reg2->i + reg3->i;
				break;
			}
			case Ins::Type::POP: {
				break;
			}
			case Ins::Type::RET: {
				cout << "ret value" << regs["eax"]->i << endl;
				pc = *(reinterpret_cast<int *>(stack + regs["ebp"]->i + 4));
				if (pc == -1) {
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
				env = env->parent;
				break;

			}
			case Ins::Type::JMP: {
				auto label = ins.operNum[0];
				pc = labelPos[label->reg];
				break;
			}
			case Ins::Type::CALL: {
				auto label = ins.operNum[0];
				if (label->reg == "print") {
					auto arg = *(int *)(stack + regs["esp"]->i + 4);
					cout << "print " << arg << endl;
				}
				else {
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
				assert(0);
				break;
			}
				pc++;

		}
	}
private:
	vector<Ins> text;
	int i;
	char *stack;
	int pc;
	map<string, Num*> regs;
	regEnv *env;
	map<string, int> labelPos;
};
void parse(Vm &v, string filename) {
	ifstream f(filename);
	if (!f.is_open()) {
		cout << "not open" << endl;
	}
	char buf[1024];
	auto split = [](string line) {
		vector<string>res;
		int i = 0;
		int j = 0;
		while (i < line.length()) {
			if (line[j] == '#') {
				return res;
			}
			while (j < line.length() && !(isspace(line[j]) || line[j] == ',')) {
				j++;
			}
			res.emplace_back(line.substr(i, j - i));
			//cout << line.substr(i, j) << endl;
			while (j < line.length() && (isspace(line[j]) || line[j] == ',')) {
				j++;
			}
			i = j;
			//cout << i << endl;
		}
		return res;
	};
	while (f.getline(buf, 1024)) {
		string line{ buf };
		//cout << line << endl;
		//if (line.substr(0,3)=="BEG")
		if (line == "") {
			continue;
		}
		//for (auto s : split(line)) {
		//	cout <<s<<"\t";
		//}
		//cout << endl;
		auto insLine = split(line);
		if (insLine[0] == "") {
			continue;
		}
		if (insLine[0] == "BEGIN") {
			v.addLabel(insLine[1]);
		}
		else if (insLine[0] == "END") {
			continue;
		}
		else if (insLine[0][0] == 'L'&& insLine[0].back() == ':') {
			auto s = insLine[0];
			s.pop_back();
			v.addLabel(s);
		}
		else if (insLine[0] == "sub") {
			int pos = insLine[2].find('-');
			if (pos == -1) {
				v.addSub(insLine[1], insLine[1], insLine[2]);
			}
			else {
				auto s2 = insLine[2].substr(0, pos);
				auto s3 = insLine[2].substr(pos + 1);
				v.addSub(insLine[1], s2, s3);
			}
		}
		else if (insLine[0] == "add") {
			int pos = insLine[2].find('+');
			if (pos == -1) {
				v.addAdd(insLine[1], insLine[1], insLine[2]);
			}
			else {
				auto s2 = insLine[2].substr(0, pos);
				auto s3 = insLine[2].substr(pos + 1);
				v.addAdd(insLine[1], s2, s3);
			}
		}
		else if (insLine[0] == "mov") {
			if (insLine[1].find('[') == -1) {
				if (isdigit(insLine[2][0])) {
					v.addMOVCONST2REG(insLine[1], insLine[2]);
				}
				else if (insLine[2][0] == '[') {
					auto tmp = insLine[2];
					tmp.pop_back();
					auto pos = tmp.find('+');
					auto s2 = tmp.substr(1, pos - 1);
					auto s3 = tmp.substr(pos + 1);
					v.addMOVMEM2REG(insLine[1], s2, s3);
				}
				else {
					v.addMOVREG2REG(insLine[1], insLine[2]);
				}
			}
			else {
				auto tmp = insLine[1];
				tmp.pop_back();
				auto pos = tmp.find('+');
				auto s2 = tmp.substr(1, pos - 1);
				auto s3 = tmp.substr(pos + 1);
				v.addMOVREG2MEM(s2, s3, insLine[2]);
			}
		}
		else if (insLine[0] == "push") {
			v.addPush(insLine[1]);
		}
		else if (insLine[0] == "add") {
		}
		else if (insLine[0] == "pop") {
		}
		else if (insLine[0] == "ret") {
			v.addRET();
		}
		else if (insLine[0] == "jmp") {
			auto label = insLine[1];
			v.addJMP(label);
		}
		else if (insLine[0] == "call") {
			v.addCALL(insLine[1]);
		}
		else if (insLine[0] == "cmp") {
			v.addCMP(insLine[1], insLine[2]);
		}
		else if (insLine[0] == "je") {
			v.addJE(insLine[1]);
		}
		else if (insLine[0] == "mul") {
			{
				int pos = insLine[2].find('*');
				if (pos == -1) {
					v.addMul(insLine[1], insLine[1], insLine[2]);
				}
				else {
					auto s2 = insLine[2].substr(0, pos);
					auto s3 = insLine[2].substr(pos + 1);
					v.addMul(insLine[1], s2, s3);
				}
			}
			//return;
			cout << "___________________\n" << endl;

			//cout << "1" << endl;
			//break;
		}
		else {
			cout << "ass " << insLine[0] << endl;
			assert(0);
		}
	}
}
int main() {
	auto vm = Vm();
	/*parse(vm, "c:/Users/ssk/Source/Repos/tigerRose/chap4/chap4/fac_1.txt");*/
	parse(vm, "c:/Users/ssk/Source/Repos/tigerRose/chap4/chap4/fac_iter.txt");
	vm.Run();
}