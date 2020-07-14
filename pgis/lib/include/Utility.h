#ifndef PGIS_UTILITY_H
#define PGIS_UTILITY_H

#include "unistd.h"

#include <cerrno>
#include <cstring>

namespace {
inline std::string getHostname() {
  char *cName = (char *)malloc(255 * sizeof(char));
  if (!gethostname(cName, 255)) {
    std::cerr << "Error determining hostname. errno = " << errno << '\n' << std::strerror(errno) << std::endl;
    std::cout << "Determined hostname: " << cName << std::endl;
  } else {
    std::cout << cName << std::endl;
  }
  std::string name(cName);
  free(cName);
  return name;
}
}  // namespace

#endif
