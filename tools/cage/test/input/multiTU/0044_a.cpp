extern int foo();

int bar() { return foo(); }

__attribute__((retain)) int har() { return bar(); }

int main() { return 0; }