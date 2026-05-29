struct Base {};

int main(int argc, char** argv) {
  Base* bp = new Base();
  delete bp;
  return 0;
}
