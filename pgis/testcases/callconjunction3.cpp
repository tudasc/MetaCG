
#include <cstdio>
void b()__attribute__((noinline));
void a()__attribute__((noinline));
void bar()__attribute__((noinline));
void foo()__attribute__((noinline));
void c()__attribute__((noinline));
void e()__attribute__((noinline));
void d()__attribute__((noinline));


void d(){
	e();
}

void e(){

}

void c(){
	b();
	d();
}

void b(){
	printf("Hello world");
	e();
}

void a(){
	b();
}

void bar(){
	b();
}

void foo(){
	a();
	bar();
}

int main(int argc, char** argv){

	printf("Little test generating another callconjunction.\n");
	foo();
	c();
return 0;
}
