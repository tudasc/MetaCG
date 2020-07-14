
#include <cstdio>
void a1(int j)__attribute__((noinline));
void a2(int j)__attribute__((noinline));
void a3(int j)__attribute__((noinline));
void a4(int j)__attribute__((noinline));

void b1()__attribute__((noinline));
void b2(int j)__attribute__((noinline));
void b3(int j)__attribute__((noinline));
void b4(int j)__attribute__((noinline));

void bar()__attribute__((noinline));
void foo()__attribute__((noinline));
void c()__attribute__((noinline));

#define FOR(n, j) if(!j) for(int i=0; i<n; i++)

void c(){
}

void b1(){
	FOR(3000,false) {
		b2(i);
		bar();
	}
}
void b2(int j){
	FOR(5000,j) {
		b3(i);
	}
}
void b3(int j){
	FOR(4000,j) {
		b4(i);
	}
}
void b4(int i){
	c();
}

void a1(int j){
	FOR(2000,j) {
		a2(i);
	}
}
void a2(int j){
	FOR(4000,j) {
		a3(i);
	}
}
void a3(int j){
	FOR(3000,j) {
	a4(i);
	}
}
void a4(int i){
	c();
}

void bar(){
}

void foo(){
	FOR(2000,false) {
		a1(i);
	}
	b1();
}

int main(int argc, char** argv){

	printf("Let's test the MoveInstrumentationUpwards phase.\n");
	foo();
return 0;
}
