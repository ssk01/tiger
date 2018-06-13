
#include "parse.h"
using namespace std;

int main() {
	Vm vm;
	//parse(vm, "c:/Users/ssk/Source/Repos/tigerRose/chap4/chap4/fac_1.txt");
	/*parse(vm, "c:/Users/ssk/Source/Repos/tigerRose/chap4/chap4/test2.txt");*/
	parse(vm, "c:/Users/ssk/Source/Repos/tigerRose/chap4/chap4/ssktest/test12.txt");
	//parse(vm, "c:/Users/ssk/Source/Repos/tigerRose/chap4/chap4/ssktest/test12.txt");
	vm.Run();
	//auto b = new int[0];

	int a = 3;
	//printf("adfsd%x...\n", out);
	//delete[] out;
	/*int a;
	cout << &a << endl;
	cout << new int(10) << endl;
	cout << new int[10] << endl;*/
}