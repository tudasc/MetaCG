#ifndef CGCOLLECTOR_GLOBALCALLDEPTH_H
#define CGCOLLECTOR_GLOBALCALLDEPTH_H

#include <nlohmann/json.hpp>

void calculateGlobalCallDepth(nlohmann::json& j, bool useOnlyMainEntry);

#endif  // CGCOLLECTOR_GLOBALCALLDEPTH_H
