// This checks just that we can handle calls to variadic functions, not that their arguments are handled correctly

extern int var_func(int a, int, int b, ...);

int main() {
  int arga = 0;
  int argb = 0;
  int argc = 0;
  int argd = 0;
  var_func(arga, argb, argc, argd);
  return 0;
}
