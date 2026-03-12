/**
* File: Plugin.cpp
* License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
* https://github.com/tudasc/metacg/LICENSE.txt
*/

#include "Plugin.h"
#include "LoggerUtil.h"

#include "llvm/Support/DynamicLibrary.h"

Plugin* loadPlugin(const std::string& pluginPath) {
  metacg::MCGLogger::instance().getConsole()->debug("Loading plugin");
  std::string err;
  auto lib = llvm::sys::DynamicLibrary::getPermanentLibrary(pluginPath.c_str(), &err);
  if (!lib.isValid()) {
    metacg::MCGLogger::instance().getErrConsole()->error("cannot locate the library at {}!", pluginPath);
    metacg::MCGLogger::instance().getErrConsole()->error("Reason: {}", err);
    return nullptr;
  }
  metacg::MCGLogger::instance().getConsole()->trace("Getting collection object from plugin {}", pluginPath);
  void* sym = lib.getAddressOfSymbol("getPlugin");
  if (!sym) {
    metacg::MCGLogger::instance().getErrConsole()->error(
        "Could not load collectors from plugin, no Function \"getPlugin()\"!");
    return nullptr;
  }
  auto getPlugin = reinterpret_cast<Plugin* (*)()>(sym);
  Plugin* loadedPlugin=getPlugin();
  metacg::MCGLogger::logInfo("Successfully loaded Plugin: {}", loadedPlugin->getPluginName());
  return loadedPlugin;
}
