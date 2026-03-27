/**
* File: CaGeDemoPlugin.cpp
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "CaGeDemoMD.h"
#include "cage/interface/CaGePlugin.h"

#include "metacg/Callgraph.h"

/**
 * This Plugin showcases how to work with CaGe's Plugin interface and MetaCG's custom Metadata.
 * To keep this plugin simple we do not go into clang/llvm internals
 */
struct CaGeDemoPlugin final : cage::Plugin {

    /**
   * Overwrite if you need to modify the graph, e.g. adding nodes, edges or metadata, based on the LLVM Module
   * This will be called before any consumers can consume the graph
   * For multiple plugins, invocations of this function are executed in the order the plugins were registered
   * @param m the full llvm::Module as available to the pass
   * @param cg the metacg callgraph to be modified
   * @return void
   */
    virtual void augmentCallGraph(const llvm::Module& m, metacg::Callgraph& cg) {
        //We cant do much with an llvm::Module reference without linking to llvm's libs
        //We attach the adress of the module as global metadata to the graph
        cg.addMetaData(std::make_unique<CaGeDemoMD>(&m));
    }

    /**
     * Overwrite this if you only need to read from the graph, e.g. converting to different formats, or graph analysis
     * @param cg the metacg callgraph to be read from
     */
    void consumeCallGraph(const metacg::Callgraph& cg) final {
        //We don't consume the graph
    }

    /**
     * Overwrite this if you want your Plugin to be listed with a name in the debug logs
     * @return the logging name of your plugin
     */
    [[nodiscard]] virtual std::string getPluginName() const { return "Cage Demo Plugin"; }
};

extern "C" {
    cage::Plugin* getPlugin() {
        return new CaGeDemoPlugin();
    }
}