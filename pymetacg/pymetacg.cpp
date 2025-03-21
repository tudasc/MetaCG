#include <nanobind/nanobind.h>

#include <MCGManager.h>

int add(int a, int b) { return a + b; }

NB_MODULE(pymetacg, m) {
    m.def("add", &add);
}
