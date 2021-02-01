#include "loadImbalance/LIRetriever.h"

using namespace LoadImbalance;

bool LIRetriever::handles(const CgNodePtr n) { return n->has<LoadImbalance::LIMetaData>(); }

LoadImbalance::LIMetaData LIRetriever::value(const CgNodePtr n) { return *(n->get<LoadImbalance::LIMetaData>()); }

std::string LIRetriever::toolName() { return "LIData"; }