
#include <cstdio>
void b()__attribute__((noinline));
void a()__attribute__((noinline));
void bar()__attribute__((noinline));
void foo()__attribute__((noinline));


void b(){
	printf("Hello world");
}

void a(){
	b();
}

void bar(){
	b();
}

void foo(){
	a();
	a();
	bar();
	bar();
}

int main(int argc, char** argv){

	printf("Little test generating call conjunction.\n");
	foo();
return 0;
}
