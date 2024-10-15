// call to multiple functions

int lastChild() { return 1; }

int middle2() { return lastChild(); }

int middle() { return middle2(); }

int main(int argc, char* argv[]) {
  middle();
  lastChild();
  return 0;
}
