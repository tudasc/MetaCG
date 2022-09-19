// From the Precise Call Graphs for C Programs with Function Pointers paper
// Tests function pointer dispatch via array and casts to void ptr

typedef int (*PFB)();

extern int strcmp(const char *, const char *);

struct parse_table {
  const char *name;
  void *func;
};
int func1() { return 0; }
int func2() { return 1; }
struct parse_table table[] = {{"name1", (void *)&func1}, {"name2", (void *)&func2}};
void *find_p_func(char *s) {
  for (int i = 0; i < 2; i++)
    if (strcmp(table[i].name, s) == 0)
      return table[i].func;
  return 0;
}
int main(int argc, char *argv[]) {
  PFB parse_func = (PFB)find_p_func(argv[1]);
  (*(parse_func))();
}
