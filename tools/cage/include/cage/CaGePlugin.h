/**
 * File: CallGraphConsumer.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */
#ifndef METACG_CAGEPLUGIN_H
#define METACG_CAGEPLUGIN_H

#include <string> //can not predeclare std::string

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
  virtual void augmentCallGraph(llvm::Module&, metacg::Callgraph&) {}
  virtual void consumeCallGraph(metacg::Callgraph&) {};
  /**
   * Overwrite this if you want your Plugin to be listed with a name in the debug logs
   * @return the logging name of your plugin
   */
  virtual std::string getPluginName() const{ return "unnamed Plugin"; }
};

}  // namespace cage

#endif  // METACG_CAGEPLUGIN_H
