/**
* File: Plugin.cpp
* License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
* https://github.com/tudasc/metacg/LICENSE.txt
*/

#include "cgcollector2/interface/Plugin.h"
#include "metacg/LoggerUtil.h"

#include "llvm/Support/DynamicLibrary.h"

cgcollector2::Plugin* loadPlugin(const std::string& pluginPath) {
  metacg::MCGLogger::logDebug("Loading plugin");
  std::string err;
  auto lib = llvm::sys::DynamicLibrary::getPermanentLibrary(pluginPath.c_str(), &err);
  if (!lib.isValid()) {
    metacg::MCGLogger::logError("cannot locate the library at {}!", pluginPath);
    metacg::MCGLogger::logError("Reason: {}", err);
    return nullptr;
  }
  metacg::MCGLogger::logTrace("Getting collection object from plugin {}", pluginPath);
  void* sym = lib.getAddressOfSymbol("getCGCollector2Plugin");
  if (!sym) {
    metacg::MCGLogger::logError("Could not load collectors from plugin, no Function \"getCGCollector2Plugin()\"!");
    return nullptr;
  }
  cgcollector2::Plugin* loadedPlugin = reinterpret_cast<cgcollector2::Plugin* (*)()>(sym)();
  metacg::MCGLogger::logInfo("Successfully loaded Plugin: {}", loadedPlugin->getPluginName());
  return loadedPlugin;
}
