/**
* File: CGC2DemoPlugin.cpp
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "CGC2DemoMD.h"
#include "cgcollector2/interface/CGC2Plugin.h"

#include "Callgraph.h"

/**
 * This Plugin showcases how to work with CGC2's Plugin interface and MetaCG's custom Metadata.
 * To keep this plugin simple we do not go into clang/llvm internals
 */
struct CGC2DemoPlugin final : cgcollector2::Plugin {
    /**
    * We attach the given function pointer as a 64 bit value to the graph
    * @param  f a FunctionDecl pointer to a non-owned read only clang function declaration
    * @return your custom metadata (needs to inherit from this toplevel class)
    **/
    std::unique_ptr<metacg::MetaData> computeForDecl([[maybe_unused]] const clang::FunctionDecl* const f) final {
        return std::make_unique<CGC2DemoMD>(f);
    }

    /**
     * Overwrite if you compute metadata that needs other metadata or has an inter functional computation scope
     * This will be called after all other @computeForDecl() calls finished
     * You are expected to attach all metadata yourself
     * @param cg the MetaCG callgraph
     * @return void
     */
    void computeForGraph([[maybe_unused]] metacg::Callgraph* const cg) final {
        //We want to add the sum of the addresses of each caller and callee to the edge
        for (auto& edge : cg->getEdges()) {
            const auto& callerID = edge.first.first;
            const auto& calleeID = edge.first.second;
            const auto& caller = *cg->getNode(callerID);
            const auto& callee = *cg->getNode(calleeID);
            const auto& callerValue = caller.get<CGC2DemoMD>()->getAdressValue();
            const auto& calleeValue = callee.get<CGC2DemoMD>()->getAdressValue();
            cg->addEdgeMetaData(caller, callee, std::make_unique<CGC2DemoMD>(callerValue + calleeValue));
        }

        //We then add the sum of all addresses and add it as global metadata to the graph
        uintptr_t sum = 0;
        for (auto& node : cg->getNodes()) {
            sum += node->get<CGC2DemoMD>()->getAdressValue();
        }
        cg->addMetaData(std::make_unique<CGC2DemoMD>(sum));
    }

    /**
     * Overwrite this if you want your Plugin to be listed with a name in the debug logs
     * @return the logging name of your plugin
     */
    std::string getPluginName() const final { return "CGC2 Demo Plugin"; }
    ~CGC2DemoPlugin() final = default;
};

extern "C" {
  cgcollector2::Plugin* getPlugin(){return new CGC2DemoPlugin();}
}
