/**
 * File: UtilTest.cpp
 * License: Part of the metacg project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "Util.h"
#include "gtest/gtest.h"

TEST(UtilTest, string_split) {
  std::string verStr{"0.2.0"};
  auto verParts = metacg::util::string_split(verStr);
  ASSERT_EQ(verParts.size(), 3);
  ASSERT_EQ(verParts[0], "0");
  ASSERT_EQ(verParts[1], "2");
  ASSERT_EQ(verParts[2], "0");
}

TEST(UtilTest, string_split2) {
  std::string verStr{"1.0"};
  auto verParts = metacg::util::string_split(verStr);
  ASSERT_EQ(verParts.size(), 2);
  ASSERT_EQ(verParts[0], "1");
  ASSERT_EQ(verParts[1], "0");
}

TEST(UtilTest, getVersionNumberAtPosition) {
  std::string verStr{"1.0"};
  auto verNumber = metacg::util::getVersionNoAtPosition(verStr, 0);
  ASSERT_EQ(verNumber, 1);
  verNumber = metacg::util::getVersionNoAtPosition(verStr, 1);
  ASSERT_EQ(verNumber, 0);
}

TEST(UtilTest, getMajorVersionNumber) {
  std::string verStr{"1.0"};
  auto verNumber = metacg::util::getMajorVersionFromString(verStr);
  ASSERT_EQ(verNumber, 1);
}

TEST(UtilTest, getMinorVersionNumber) {
  std::string verStr{"1.0"};
  auto verNumber = metacg::util::getMinorVersionFromString(verStr);
  ASSERT_EQ(verNumber, 0);
}
