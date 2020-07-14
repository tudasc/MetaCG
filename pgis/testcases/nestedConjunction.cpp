
#include <cstdio>
void a()__attribute__((noinline));
void b()__attribute__((noinline));
void c()__attribute__((noinline));
void d()__attribute__((noinline));
void print()__attribute__((noinline));

void d(){
	print();
}

void c(){
	print();
}

void b(){
	d();
	d();
}

void a(){
	d();
	d();
}

void print(){
	printf("hello\n");
}

int main(int argc, char** argv){

	c();
	c();
	c();
	c();

	a();
	b();

	return 0;
}
