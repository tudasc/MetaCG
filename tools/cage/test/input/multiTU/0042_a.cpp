__attribute__((retain))
int foo() { return 42; }

__attribute__((retain))
int baz() { return foo(); }

int main() { return 0; }