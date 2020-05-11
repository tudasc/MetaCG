
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

CallgraphManager build(std::string filePath, Config *c);
// float CalculateRuntimeThreshold(std::string filePath, Config* c);
float CalculateRuntimeThreshold(CallgraphManager *cg);
void buildFromCube(std::string filePath, Config *c, CallgraphManager &cg);
float bucket_sort(float *, int);
}  // namespace CubeCallgraphBuilder

#endif
