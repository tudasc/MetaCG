// call to multiple functions

int lastChild() { return 1; }

int middle() { return lastChild(); }

__attribute__((retain)) int middle2() { return lastChild(); }

int main(int argc, char* argv[]) {
  middle();
  return 0;
}
