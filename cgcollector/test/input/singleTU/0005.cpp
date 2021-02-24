// two functions called independently from main

void childOne() {}
void childTwo() {}

int main(int argc, char *argv[]) {
  childOne();
  childTwo();

  return 0;
}
