/**
 * File: pymetacg.cpp
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "nanobind_json.h"  // IWYU pragma: keep
#include <nanobind/make_iterator.h>
#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/unique_ptr.h>
#include <nanobind/stl/unordered_map.h>
#include <nanobind/stl/unordered_set.h>

#include <Callgraph.h>
#include <CgNode.h>
#include <MCGManager.h>

#include <io/MCGReader.h>
#include <io/MCGWriter.h>
#include <io/VersionFourMCGReader.h>
#include <metadata/MetaData.h>

// BuiltinMD.h is not used directly here, but including this header
// ensures that MetaCG's built-in metadata types become part of this
// translation unit and are hence included in the pymetacg dynamic library.
#include <metadata/BuiltinMD.h>  // IWYU pragma: keep

#include <string>

#include "util.h"

#include <config.h>

namespace nb = nanobind;
using namespace metacg::pymetacg;

NB_MODULE(pymetacg, m) {
  m.attr("info") = std::string("MetaCG v" + std::to_string(MetaCG_VERSION_MAJOR) + "." +
                               std::to_string(MetaCG_VERSION_MINOR) + " (" + MetaCG_GIT_SHA + ")");

  nb::class_<MetaDataWrapper>(m, "MetaData")
      .def_prop_ro("key", [](const MetaDataWrapper& self) { return self.md->getKey(); })
      .def_prop_ro("data", [](const MetaDataWrapper& self) {
        NameMapping mapping(self.graph);
        return self.md->toJson(mapping);
      });

  nb::class_<MetaDataContainer>(m, "MetaDataContainer")
      .def("__getitem__",
           [](const MetaDataContainer& self, const std::string& key) {
             try {
               return MetaDataWrapper{self.map.at(key).get(), self.graph};
             } catch (std::out_of_range& e) {
               throw nb::key_error("Meta data entry not found in node.");
             }
           })
      .def("__contains__",
           [](const MetaDataContainer& self, const std::string& key) { return self.map.find(key) != self.map.end(); })
      .def("__len__", [](const MetaDataContainer& self) { return self.map.size(); });

  nb::class_<CgNodeWrapper>(m, "CgNode")
      .def_prop_ro("function_name", [](const CgNodeWrapper& self) { return self.node->getFunctionName(); })
      .def_prop_ro(
          "meta_data",
          [](const CgNodeWrapper& self) { return MetaDataContainer{self.node->getMetaDataContainer(), self.graph}; })
      .def("__repr__",
           [](const CgNodeWrapper& self) { return std::string("CgNode(") + self.node->getFunctionName() + ")"; })
      .def("__hash__", [](const CgNodeWrapper& self) { return self.node->getId(); })
      .def("__eq__",
           [](const CgNodeWrapper& self, CgNodeWrapper& other) { return self.node->getId() == other.node->getId(); })
      .def_prop_ro("callers",
                   [](const CgNodeWrapper& self) {
                     return attachGraphPointerToNodes(self.graph.getCallers(*self.node), self.graph);
                   })
      .def_prop_ro("callees", [](const CgNodeWrapper& self) {
        return attachGraphPointerToNodes(self.graph.getCallees(*self.node), self.graph);
      });

  nb::class_<metacg::Callgraph>(m, "Callgraph")
      .def_static("from_file",
                  [](std::string& path) {
                    metacg::io::FileSource fs(path);

                    auto mcgReader = metacg::io::createReader(fs);
                    auto graph = mcgReader->read();
                    return graph;
                  })
      .def("__iter__",
           [](const metacg::Callgraph& self) {
             return nb::make_iterator(nb::type<CgNodeWrapper>(), "nodes",
                                      NodeContainerIteratorWrapper(self.getNodes().begin(), self),
                                      NodeContainerIteratorWrapper(self.getNodes().end(), self));
           })
      .def("__len__", [](const metacg::Callgraph& self) { return self.getNodeCount(); })
      .def("get_nodes",
           [](const metacg::Callgraph& self, const std::string& name) {
             const auto& nodes = self.getNodes(name);
             if (nodes.size() == 0) {
               throw nb::key_error("Node not found in callgraph.");
             } else {
               return nb::make_iterator(nb::type<CgNodeWrapper>(), "nodes",
                                        NodeListIteratorWrapper(nodes.begin(), self),
                                        NodeListIteratorWrapper(nodes.end(), self));
             }
           })
      .def("get_first_node",
           [](const metacg::Callgraph& self, const std::string& name) {
             metacg::CgNode* node = self.getFirstNode(name);
             if (node == nullptr) {
               throw nb::key_error("Node not found in callgraph.");
             } else {
               return CgNodeWrapper{node, self};
             }
           })
      .def("get_single_node",
           [](const metacg::Callgraph& self, const std::string& name) {
             const auto& nodes = self.getNodes(name);
             const size_t n = nodes.size();

             if (n == 0) {
               throw nb::key_error("Node not found in callgraph.");
             } else if (n > 1) {
               throw nb::key_error("More than one node with name found in callgraph.");
             } else {
               return CgNodeWrapper{self.getNode(nodes[0]), self};
             }
           })
      .def("__getitem__",
           [](const metacg::Callgraph& self, const std::string& name) {
             const auto& nodes = self.getNodes(name);
             const size_t n = nodes.size();

             if (n == 0) {
               throw nb::key_error("Node not found in callgraph.");
             } else if (n > 1) {
               throw nb::key_error("More than one node with name found in callgraph.");
             } else {
               return CgNodeWrapper{self.getNode(nodes[0]), self};
             }
           })
      .def("__contains__", [](const metacg::Callgraph& self, const std::string& key) { return self.hasNode(key); });
}
