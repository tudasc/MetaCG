int foo() { return 2; }

__attribute__((retain)) int hf() { return foo(); }
