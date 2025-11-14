// call to multiple functions

int lastChild() { return 1; }

int middle() { return lastChild(); }

int middle2() { return lastChild(); }

int main(int argc, char* argv[]) {
  middle();
  return 0;
}
