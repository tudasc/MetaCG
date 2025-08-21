/**
* File: Plugin.cpp
* License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
* https://github.com/tudasc/metacg/LICENSE.txt
*/

#include "Plugin.h"
#include "LoggerUtil.h"

#include <dlfcn.h>

Plugin* loadPlugin(const std::string& pluginPath) {
  metacg::MCGLogger::instance().getConsole()->debug("Loading plugin");
  void* handle = dlopen(pluginPath.c_str(), 1);
  if (!handle) {
    metacg::MCGLogger::instance().getErrConsole()->error("cannot locate the library at {}!", pluginPath);
    return nullptr;
  }
  metacg::MCGLogger::instance().getConsole()->trace("Getting collection object from plugin {}", pluginPath);
  auto (*getPlugin)() = (Plugin * (*)()) dlsym(handle, "getPlugin");
  if (!getPlugin) {
    metacg::MCGLogger::instance().getErrConsole()->error(
        "Could not load collectors from plugin, no Function \"getPlugin()\"!");
    return nullptr;
  }

  Plugin* loadedPlugin=getPlugin();
  metacg::MCGLogger::logInfo("Successfully loaded Plugin: {}", loadedPlugin->getPluginName());
  return loadedPlugin;
}