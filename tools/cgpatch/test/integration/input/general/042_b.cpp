extern void say_hello();
int main() {
  void (*fptr)();    // declare function pointer
  fptr = say_hello;  // assign the function to the pointer
  fptr();
  return 0;
}
