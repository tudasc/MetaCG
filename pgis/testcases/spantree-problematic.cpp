
#include <cstdio>
void printFoo()__attribute__((noinline));
void printBar()__attribute__((noinline));
void a()__attribute__((noinline));
void b()__attribute__((noinline));

void printFoo(){
//	printf("Hello Foo\n");
}

void printBar(){
//	printf("Hello Bar\n");
}

void a(){
	for(int i=0; i<100000000; i++) {
		printFoo();
		printBar();
	}
}

void b(){
	for(int i=0; i<100000000; i++) {
		printFoo();
		printBar();
	}
}

int main(int argc, char** argv){

	a();
	b();

return 0;
}
