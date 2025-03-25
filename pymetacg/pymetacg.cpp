#include <string>
#include <fstream>

#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/unique_ptr.h>

#include <MCGManager.h>
#include <Callgraph.h>
#include <CgNode.h>

#include <io/MCGReader.h>
#include <io/VersionThreeMCGReader.h>
#include <io/MCGWriter.h>

namespace nb = nanobind;
using namespace nb::literals;

NB_MODULE(pymetacg, m) {
    // nb::class_<metacg::graph::MCGManager>(m, "MCGManager");

    nb::class_<metacg::CgNode>(m, "CgNode")
        .def_prop_ro("function_name", &metacg::CgNode::getFunctionName);

    nb::class_<metacg::Callgraph>(m, "Callgraph")
        .def_static("from_file", [](std::string& path) {
            metacg::io::FileSource fs(path);

            auto mcgReader = metacg::io::createReader(fs);
            auto graph = mcgReader->read();
            return graph;
        })
        .def("write_to_file", [](const metacg::Callgraph& self, const std::string& path) {
            auto mcgWriter = metacg::io::createWriter(3);
            metacg::io::JsonSink jsonSink;
            mcgWriter->write(&self, jsonSink);
            std::ofstream os(path);
            os << jsonSink.getJson() << std::endl;
        })
        .def("has_node", [](const metacg::Callgraph& self, const std::string& name) {
            return self.hasNode(name);
        })
        .def("get_node", [](const metacg::Callgraph& self, const std::string& name) {
            return self.getNode(name);
        })
        .def("clear", [](metacg::Callgraph& self) {
            self.clear();
        });
}
