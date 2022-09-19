// Test for handling function ptrs in a global variable across different TUs
void function() { int a = 1 + 2; }

void *functionptr;

int foo() {
  void *var = (void *)&function;
  functionptr = var;
  return 1;
}