
#include <cstdio>
void print()__attribute__((noinline));
void a(int i)__attribute__((noinline));
void b()__attribute__((noinline));

void print(){
	printf("Hello world\n");
}

void a(int i){
	if(i==0) {
		print();
	} else {
		a(i-1);
	}
}

void b(){
	print();
}

int main(int argc, char** argv){

	printf("Little test generating call conjunction.\n");

	a(1);
	b();

return 0;
}
