
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
	a();
}

void foo(){
	bar();
}

int main(int argc, char** argv){

	printf("Little test generating a call chain.\n");
	foo();
return 0;
}
