/**
* File: Plugin.h
* License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
* https://github.com/tudasc/metacg/LICENSE.txt
*/

#ifndef CGCOLLECTOR2_PLUGIN_H
#define CGCOLLECTOR2_PLUGIN_H

/**
 * This is the base class from which all plugins inherit
 * A plugin can either do computation on a function decl, or on the generated graph
 * The first graph based calculations will start once all decleration based calculations are completed
 * This means graph based calculations can expect metadata to be available
 * There are *no ordering guarantees given* for samely typed computations
 */

#include <string>

namespace metacg {
struct MetaData;
class Callgraph;
}  // namespace metacg

namespace clang {
class FunctionDecl;
}

struct Plugin {
  explicit Plugin() {}

  /**
   * Overwrite if you compute metadata for a single function declarations
   * There is no guaranteed ordering of declarations
   * for this use @computeForGraph() instead
   * @param functionDecl a pointer to a non owned read only clang function declaration
   * @return your custom metadata (needs to inherit from this toplevel class)
   **/
  virtual metacg::MetaData* computeForDecl([[maybe_unused]] const clang::FunctionDecl* const) { return nullptr; };

  /**
   * Overwrite if you compute metadata that needs other metadata or has a inter functional computation scope
   * This will be called after all other @computeForDecl() calls finished
   * You are expected to attach all metadata yourself
   * @param cg
   * @return void
   */
  virtual void computeForGraph([[maybe_unused]] const metacg::Callgraph* const) {};

  /**
   * Overwrite this if you want your Plugin to be listed with a name in the debug logs
   * @return the logging name of your plugin
   */
  virtual std::string getPluginName() { return "unnamed Plugin"; }
  virtual ~Plugin() = default;
};

// Todo: This currently only loads one plugin per *.so
Plugin* loadPlugin(const std::string& pluginPath);

#endif  // CGCOLLECTOR2_PLUGIN_H
