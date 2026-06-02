#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void L0(void);
int main(void) { L0(); return 0; }

void Tiger_exit(long code) { fflush(stdout); exit((int)code); }

void printInt(long n) { printf("%ld\n", n); }
void print(char *s) {
	if (strcmp(s, "newline") == 0) printf("\n");
	else printf("%s", s);
}
long ord(char *s) { return (long)(unsigned char)s[0]; }
long stringEqual(char *a, char *b) { return strcmp(a, b) == 0; }
void *malloc_tiger(long size) { return malloc(size); }
void *initArray(long size, long init) {
	long *arr = (long *)malloc(size * sizeof(long));
	for (long i = 0; i < size; i++) arr[i] = init;
	return arr;
}
