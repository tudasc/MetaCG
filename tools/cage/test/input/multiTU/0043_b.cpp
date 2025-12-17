extern int bar();

__attribute__((retain))
int har() { return 4; }

__attribute__((retain))
int goo() { return bar(); }
