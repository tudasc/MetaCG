/**
 * File: LIMetaDataTest.cpp
 * License: Part of the metacg project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "../../../../graph/include/Callgraph.h"
#include "gtest/gtest.h"
#include <MCGWriter.h>
#include <loadImbalance/LIRetriever.h>

using namespace metacg;

TEST(LIMetaDataTest, NoAnnotation) {
  Callgraph c;
  nlohmann::json j;
  LoadImbalance::LIRetriever pr;
  int annotCount = metacg::io::doAnnotate(c, pr, j);
  ASSERT_EQ(0, annotCount);
}

TEST(LIMetaDataTest, SimpleAnnotation) {
  nlohmann::json j;
  j["main"] = {{"numStatements", 42}, {"doesOverride", false}, {"hasBody", true}};

  auto n = std::make_shared<CgNode>("main");
  getOrCreateMD<LoadImbalance::LIMetaData>(n);
  getOrCreateMD<pira::PiraOneData>(n);
  n->get<LoadImbalance::LIMetaData>()->flag(LoadImbalance::FlagType::Visited);

  Callgraph c;
  c.insert(n);

  LoadImbalance::LIRetriever pr;
  int annotCount = metacg::io::doAnnotate(c, pr, j);

  ASSERT_EQ(1, annotCount);
  ASSERT_TRUE(j["main"]["meta"]["LIData"]["visited"] == true);
  ASSERT_TRUE(j["main"]["meta"]["LIData"]["irrelevant"] == false);
  ASSERT_EQ(j["main"]["numStatements"], 42);
}

TEST(LIMetaDataTest, ComplexAnnotation) {
  nlohmann::json j;
  j["main"] = {{"numStatements", 42}, {"doesOverride", false}, {"hasBody", true}};
  j["node2"] = {{"numStatements", 34}, {"doesOverride", false}, {"hasBody", true}};

  auto n = std::make_shared<CgNode>("main");
  getOrCreateMD<LoadImbalance::LIMetaData>(n);
  getOrCreateMD<pira::PiraOneData>(n);
  n->get<LoadImbalance::LIMetaData>()->flag(LoadImbalance::FlagType::Visited);

  auto n2 = std::make_shared<CgNode>("node2");
  getOrCreateMD<LoadImbalance::LIMetaData>(n2);
  getOrCreateMD<pira::PiraOneData>(n2);
  n2->get<LoadImbalance::LIMetaData>()->flag(LoadImbalance::FlagType::Irrelevant);
  n2->get<LoadImbalance::LIMetaData>()->flag(LoadImbalance::FlagType::Visited);
  n2->get<LoadImbalance::LIMetaData>()->setNumberOfInclusiveStatements(100);
  n2->get<LoadImbalance::LIMetaData>()->setVirtual(true);

  Callgraph c;
  c.insert(n);
  c.insert(n2);

  LoadImbalance::LIRetriever pr;
  int annotCount = metacg::io::doAnnotate(c, pr, j);

  ASSERT_EQ(2, annotCount);
  ASSERT_TRUE(j["main"]["meta"]["LIData"]["visited"] == true);
  ASSERT_TRUE(j["main"]["meta"]["LIData"]["irrelevant"] == false);
  ASSERT_EQ(j["main"]["numStatements"], 42);

  ASSERT_TRUE(j["node2"]["meta"]["LIData"]["visited"] == true);
  ASSERT_TRUE(j["node2"]["meta"]["LIData"]["irrelevant"] == true);
  ASSERT_EQ(j["node2"]["numStatements"], 34);
}
