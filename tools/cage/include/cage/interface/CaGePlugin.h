/**
 * File: CallGraphConsumer.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */
#ifndef METACG_CAGEPLUGIN_H
#define METACG_CAGEPLUGIN_H

#include <string> //can not predeclare std::string

#include "LoggerUtil.h"
#include "llvm/Support/DynamicLibrary.h"

namespace metacg {
class Callgraph;
class Metadata;
}

namespace llvm {
class Module;
}

namespace cage {

struct Plugin {
  virtual ~Plugin() = default;
  /**
   * Overwrite if you need to modify the graph, e.g. adding nodes, edges or metadata, based on the LLVM Module
   * This will be called before any consumers can consume the graph
   * For multiple plugins, invocations of this function are executed in the order the plugins were registered
   * @param m the full LLVM::Module as available to the pass
   * @param cg the metacg callgraph to be modified
   * @return void
   */
  virtual void augmentCallGraph(const llvm::Module& m, metacg::Callgraph& cg) {}

  /**
   * Overwrite this if you only need to read from the graph, e.g. converting to different formats, or graph analysis
   * @param cg the metacg callgraph to be read from
   */
  virtual void consumeCallGraph(const metacg::Callgraph& cg){};

  /**
   * Overwrite this if you want your Plugin to be listed with a name in the debug logs
   * @return the logging name of your plugin
   */
  [[nodiscard]] virtual std::string getPluginName() const { return "unnamed Plugin"; }
};



}  // namespace cage


#endif  // METACG_CAGEPLUGIN_H
