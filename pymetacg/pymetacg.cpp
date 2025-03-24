#include <nanobind/nanobind.h>

#include <MCGManager.h>

namespace nb = nanobind;
using namespace nb::literals;

int add(int a, int b) { return a + b; }

NB_MODULE(pymetacg, m) {
    m.def("add", &add);

    nb::class_<metacg::graph::MCGManager>(m, "MCGManager");
}
