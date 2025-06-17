
using func_t = void (*)();
extern func_t g_foo;

void call_foo() { g_foo(); }
