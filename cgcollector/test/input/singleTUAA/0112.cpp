// From the Precise Call Graphs for C Programs with Function Pointers paper
// Tests the pattern of a struct carrying customizable functions with it

struct _chunk {};
struct obstack {
  struct _chunk *chunk;
  struct _chunk *(*chunkfun)();
  void (*freefun)();
};
void chunk_fun(struct obstack *h, void *f) { h->chunkfun = (struct _chunk * (*)()) f; }
void free_fun(struct obstack *h, void *f) { h->freefun = (void (*)())f; }

extern void xfree();
extern struct _chunk *xmalloc();

int main() {
  struct obstack h;
  chunk_fun(&h, (void *)&xmalloc);
  free_fun(&h, (void *)&xfree);
  h.chunkfun();
}
