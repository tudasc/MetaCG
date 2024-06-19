/**
* File: MCGBaseInfo.cpp
* License: Part of the metacg project. Licensed under BSD 3 clause license. See LICENSE.txt file at
* https://github.com/tudasc/metacg/LICENSE.txt
*/


#include "MCGBaseInfo.h"
#include "config.h"
namespace metacg {

//This must be in its own file as to not expose config.h header to linking programs
MCGGeneratorVersionInfo getCGCollectorGeneratorInfo() {
  std::string cgcCollName("CGCollector");
  return {cgcCollName, CGCollector_VERSION_MAJOR, CGCollector_VERSION_MINOR, MetaCG_GIT_SHA};
}
}  // namespace metacg
