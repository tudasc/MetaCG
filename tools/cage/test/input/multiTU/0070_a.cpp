// Test for handling different parameters across translation units
int foo(int some_name, int);

int main() {
  foo(1, 2);
  return 0;
}