int valid() { return 0; }

int missedCallee() { return 0; }

int missedCaller() { return 0; }

int bidrectionalMiss() { return 0; }

int main(int argc, char** argv) {
  valid();
  missedCallee();
  missedCaller();
  bidrectionalMiss();
  return 0;
}
