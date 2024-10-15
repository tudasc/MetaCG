// From the Precise Call Graphs for C Programs with Function Pointers paper
// Tests the pattern of a struct carrying customizable functions with it and setting them within one function

struct _chunk {};
struct obstack {
  struct _chunk* chunk;
  struct _chunk* (*chunkfun)();
  void (*freefun)();
};
void set_fun(struct obstack* h, void* f, void* free) {
  h->chunkfun = (struct _chunk * (*)()) f;
  h->freefun = (void (*)())free;
}

extern void xfree();
extern struct _chunk* xmalloc();

int main() {
  struct obstack h;
  set_fun(&h, (void*)&xmalloc, (void*)&xfree);
  h.chunkfun();
  h.freefun();
}
