
#include <cstdio>
void print()__attribute__((noinline));
void a(int i)__attribute__((noinline));

void print(){
	for(int i=0; i<1000000; i++) {
		if(i%100000 == 0)
			printf(".");
	}
	printf("\n");
}

void a(int i){
		print();
}

int main(int argc, char** argv){

	print();

	a(1);
	a(1);

return 0;
}
