// This test is for checking that the Alias Analysis does not assume that __status and __arg are parameters to on_exit
// Tests function types in a parameter
extern int on_exit(void (*__func)(int __status, void* __arg), void* __arg);