
extern void* functionptr;

typedef void (*Ftype)();

int main() {
  ((Ftype)functionptr)();
  return 0;
}
