// Test for handling default args/ differing definitions accross translation units
extern void some_function();
extern void some_other_function();
using FType = decltype(some_function);

void f(FType n = some_function, FType arg = some_other_function);

void work() { f(); }
