
#include "CallgraphManager.h"

#include <Cube.h>
#include <CubeMetric.h>

#include <string>

#ifndef CUBEREADER_H_
#define CUBEREADER_H_

/**
 * \author roman
 * \author JPL
 */
namespace CubeCallgraphBuilder {

void build(std::string filePath, Config *c);
void buildFromCube(std::string filePath, Config *c, CallgraphManager &cg);
}  // namespace CubeCallgraphBuilder

#endif
