#include "parse.h"
using std::ifstream;
void parseString(Vm &v, vector<string>& insLine) {
	assert(insLine[0].find(':') != -1);
	insLine[0].pop_back();
	v.addString(insLine[0], insLine[1]);
}
void parseProc(Vm &v, vector<string>& insLine) {
	if (insLine[0] == "BEGIN") {
		v.addLabel(insLine[1]);
	}	
	else if (insLine[0] == "END") {
		//continue;
		return;
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
				//might fail
				v.addMOVCONST2REG(insLine[1], insLine[2]);
			}
			else if (insLine[2][0] == '[') {
				auto tmp = insLine[2];
				tmp.pop_back();
				auto pos = tmp.find('+');
				if (pos == -1) {
					v.addMOVMEM2REG(insLine[1], tmp.substr(1), string("0"));
				}
				else {
					auto s2 = tmp.substr(1, pos - 1);
					auto s3 = tmp.substr(pos + 1);
					v.addMOVMEM2REG(insLine[1], s2, s3);
				}
			}
			else {
				v.addMOVREG2REG(insLine[1], insLine[2]);
			}
		}
		else {
			auto tmp = insLine[1];
			tmp.pop_back();
			auto pos = tmp.find('+');
			if (pos == -1) {
				v.addMOVREG2MEM(tmp.substr(1), string("0"), insLine[2]);

			}
			else {
				auto s2 = tmp.substr(1, pos - 1);
				auto s3 = tmp.substr(pos + 1);
				v.addMOVREG2MEM(s2, s3, insLine[2]);
			}
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
	else if (insLine[0] == "jle") {
		auto label = insLine[1];
		v.addJLE(label);
	}
	else if (insLine[0] == "jge") {
		auto label = insLine[1];
		v.addJGE(label);
	}
	else if (insLine[0] == "jne") {
		auto label = insLine[1];
		v.addJNE(label);
	}
	else if (insLine[0] == "jg") {
		auto label = insLine[1];
		v.addJG(label);
	}
	else if (insLine[0] == "jl") {
		auto label = insLine[1];
		v.addJL(label);
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
		cout << "MISS INS " << insLine[0] << endl;
		assert(0);
	}
}
void parse(Vm &v, string filename) {
	ifstream f(filename);
	if (!f.is_open()) {
		cout << "not open" << endl;
		exit(0);
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
	bool strBlock = 0;
	while (f.getline(buf, 1024)) {
		string line{ buf };
		//cout << line << endl;
		//if (line.substr(0,3)=="BEG")
		if (line == "") {
			continue;
		}
		/*	for (auto s : split(line)) {
		cout <<s<<"\t";
		}
		cout << endl;
		continue;*/
		auto insLine = split(line);
		if (insLine.size() == 0) {
			continue;
		}
		if (insLine[0] == "") {
			continue;
		}
		if (insLine[0] == "string:") {
			strBlock = 1;
			continue;
		}
		else if (insLine[0] == "proc:") {
			strBlock = 0;
			continue;
		}
		if (strBlock == 1) {
			parseString(v, insLine);
		}
		else if (strBlock == 0) {
			parseProc(v, insLine);
		}

	}
}