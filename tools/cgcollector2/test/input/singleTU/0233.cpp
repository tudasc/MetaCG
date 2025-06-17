// Test for a new call invoving a builtin type
int main() {
  int a = 1;
  int* bp = new int(a);
  int c = *bp;
}