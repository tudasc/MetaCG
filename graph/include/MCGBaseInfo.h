/**
 * File: MCGBaseInfo.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#ifndef METACG_MCGBASEINFO_H
#define METACG_MCGBASEINFO_H

#include "config.h"
#include <string>

namespace metacg {

struct MCGFileFormatVersion {
  MCGFileFormatVersion(int major, int minor) : major(major), minor(minor) {}
  MCGFileFormatVersion(const MCGFileFormatVersion &other) = default;
  MCGFileFormatVersion(MCGFileFormatVersion &&other) = default;
  bool operator==(const MCGFileFormatVersion &rhs) const { return major == rhs.major && minor == rhs.minor; }
  bool operator!=(const MCGFileFormatVersion &rhs) const { return !(rhs == *this); }
  bool operator<(const MCGFileFormatVersion &rhs) const {
    return major < rhs.major || (major == rhs.major && minor < rhs.minor);
  }
  bool operator>(const MCGFileFormatVersion &rhs) const { return !this->operator<(rhs); }

  [[nodiscard]] std::string getVersionStr() const { return std::to_string(major) + '.' + std::to_string(minor); }

  [[nodiscard]] std::string getJsonIdentifier() const { return {"version"}; }

  int major;
  int minor;
};

struct MCGGeneratorVersionInfo {
  MCGGeneratorVersionInfo(std::string &name, int major, int minor, std::string gitSHA = {})
      : name(name), major(major), minor(minor), sha(std::move(gitSHA)) {}
  MCGGeneratorVersionInfo(const MCGGeneratorVersionInfo &other) = default;
  MCGGeneratorVersionInfo(MCGGeneratorVersionInfo &&other) = default;

  [[nodiscard]] std::string getVersionStr() const { return std::to_string(major) + '.' + std::to_string(minor); }

  [[nodiscard]] std::string getJsonIdentifier() const { return {"generator"}; }

  [[nodiscard]] std::string getJsonNameIdentifier() const { return {"name"}; }

  [[nodiscard]] std::string getJsonVersionIdentifier() const { return {"version"}; }

  [[nodiscard]] std::string getJsonShaIdentifier() const { return {"sha"}; }

  std::string name;
  int major;
  int minor;
  std::string sha;
};

struct MCGFileFormatInfo {
  MCGFileFormatInfo(int major, int minor) : version(major, minor), cgFieldName("_CG"), metaInfoFieldName("_MetaCG") {}
  MCGFileFormatInfo(const MCGFileFormatInfo &other) = default;
  MCGFileFormatInfo(MCGFileFormatInfo &&other) = default;

  MCGFileFormatVersion version;
  std::string cgFieldName;
  std::string metaInfoFieldName;
};

struct MCGFileNodeInfo {
  const std::string calleesStr{"callees"};
  const std::string isVirtualStr{"isVirtual"};
  const std::string doesOverrideStr{"doesOverride"};
  const std::string overridesStr{"overrides"};
  const std::string overriddenByStr{"overriddenBy"};
  const std::string callersStr{"callers"};
  const std::string hasBodyStr{"hasBody"};
  const std::string metaStr{"meta"};
};

struct MCGFileInfo {
  MCGFileInfo(MCGFileFormatInfo ffInfo, MCGGeneratorVersionInfo genInfo)
      : formatInfo(std::move(ffInfo)), generatorInfo(std::move(genInfo)) {}
  MCGFileInfo(const MCGFileInfo &other) = default;
  MCGFileInfo(MCGFileInfo &&other) = default;

  MCGFileFormatInfo formatInfo;
  MCGGeneratorVersionInfo generatorInfo;
  MCGFileNodeInfo nodeInfo;
};

MCGFileInfo getVersionTwoFileInfo(MCGGeneratorVersionInfo mcgGenInfo);

MCGGeneratorVersionInfo getCGCollectorGeneratorInfo();

}  // namespace metacg

#endif  // METACG_MCGBASEINFO_H
