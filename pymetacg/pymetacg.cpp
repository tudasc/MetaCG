
#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/unique_ptr.h>
#include <nanobind/stl/unordered_map.h>
#include <nanobind/stl/unordered_set.h>
#include <nanobind/make_iterator.h>
#include "nanobind_json.h" // IWYU pragma: keep

#include <MCGManager.h>
#include <Callgraph.h>
#include <CgNode.h>
#include <CgNodePtr.h>

#include <io/MCGReader.h>
#include <io/VersionThreeMCGReader.h>
#include <io/MCGWriter.h>
#include <metadata/MetaData.h>

#include <string>

#include "util.h"

namespace nb = nanobind;
using namespace metacg;

NB_MODULE(pymetacg, m) {
    nb::class_<MetaData>(m, "MetaData")
        .def_prop_ro("key", &MetaData::getKey)
        .def_prop_ro("data", [](const MetaData& self){
            return self.to_json();
        });

    nb::class_<CgNodeWrapper>(m, "CgNode")
        .def_prop_ro("function_name", [](const CgNodeWrapper& self) {return self.node->getFunctionName();})
        .def_prop_ro("meta_data", [](const CgNodeWrapper& self) {return self.node->getMetaDataContainer();})
        .def("__repr__", [](const CgNodeWrapper& self) {return std::string("CgNode(") + self.node->getFunctionName() + ")";})
        .def("__hash__", [](const CgNodeWrapper& self) {return self.node->getId();})
        .def("__eq__", [](const CgNodeWrapper& self, CgNodeWrapper& other) {return self.node->getId() == other.node->getId();})
        .def_prop_ro("callers", [](const CgNodeWrapper& self) {
            return attachGraphPointerToNodes(self.graph.getCallers(self.node), self.graph);
        })
        .def_prop_ro("callees", [](const CgNodeWrapper& self) {
            return attachGraphPointerToNodes(self.graph.getCallees(self.node), self.graph);
        });

    nb::class_<Callgraph>(m, "Callgraph")
        .def_static("from_file", [](std::string& path) {
            io::FileSource fs(path);

            auto mcgReader = io::createReader(fs);
            auto graph = mcgReader->read();
            return graph;
        })
        .def("__iter__", [](const Callgraph& self) {
            return nb::make_iterator(
                nb::type<CgNodeWrapper>(),
                "nodes", 
                UniquePtrValueIterator(self.getNodes().begin(), self),
                UniquePtrValueIterator(self.getNodes().end(), self));
        })
        .def("__getitem__", [](const Callgraph& self, const std::string& key) {
            CgNode* node = self.getNode(key);
            if (node != nullptr) {
                return CgNodeWrapper{node, self};
            } else {
                throw nb::key_error("Node not found in call graph.");
            }
        })
        .def("__contains__", [](const Callgraph& self, const std::string& key){
            return self.hasNode(key);
        });
}
