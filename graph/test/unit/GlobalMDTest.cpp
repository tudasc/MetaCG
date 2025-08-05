/**
* File: GlobalMDTest.cpp
* License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
* https://github.com/tudasc/metacg/LICENSE.txt
*/

#include "Callgraph.h"

#include "gtest/gtest.h"

#include "TestMD.h"

TEST(GlobalMD, AttachMD) {
 auto cg = std::make_unique<metacg::Callgraph>();

 auto testMd = std::make_unique<SimpleTestMD>(1337, 42.0, ":)");
 cg->addMetaData(std::move(testMd));

 EXPECT_TRUE(cg->get<SimpleTestMD>() != nullptr);
 EXPECT_EQ(cg->get<SimpleTestMD>()->stored_int, 1337);
}

TEST(GlobalMD, EraseMD) {
  auto cg = std::make_unique<metacg::Callgraph>();

  auto testMd = std::make_unique<SimpleTestMD>(1337, 42.0, ":)");
  cg->addMetaData(std::move(testMd));

  EXPECT_TRUE(cg->get<SimpleTestMD>() != nullptr);

  cg->erase<SimpleTestMD>();

  EXPECT_TRUE(cg->get<SimpleTestMD>() == nullptr);
}

TEST(GlobalMD, GetOrCreateMD) {
  auto cg = std::make_unique<metacg::Callgraph>();

  auto testMd = std::make_unique<SimpleTestMD>(1337, 42.0, ":)");
  cg->addMetaData(std::move(testMd));

  auto& md = cg->getOrCreate<SimpleTestMD>(0, 0.0, "");
  EXPECT_EQ(md.stored_int, 1337);

  cg->erase<SimpleTestMD>();
  EXPECT_TRUE(cg->get<SimpleTestMD>() == nullptr);

  auto& md2 = cg->getOrCreate<SimpleTestMD>(1, 0.0, "");
  EXPECT_EQ(md2.stored_int, 1);
}

TEST(GlobalMD, MergeMD) {
  auto cg = std::make_unique<metacg::Callgraph>();

  auto testMd = std::make_unique<SimpleTestMD>(1337, 42.0, ":)");
  cg->addMetaData(std::move(testMd));

  auto cg2 = std::make_unique<metacg::Callgraph>();
  auto testMd2 = std::make_unique<SimpleTestMD>(0, 0.0, "");

  cg->merge(*cg2, metacg::MergeByName());

  EXPECT_TRUE(cg->get<SimpleTestMD>() != nullptr);
  EXPECT_EQ(cg->get<SimpleTestMD>()->stored_int, 1337);
}
