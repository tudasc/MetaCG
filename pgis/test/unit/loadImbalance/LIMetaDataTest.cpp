#include "gtest/gtest.h"
#include <Callgraph.h>
#include <libIPCG/IPCGReader.h>
#include <loadImbalance/LIRetriever.h>

TEST(LIMetaDataTest, NoAnnotation) {
  Callgraph c;
  nlohmann::json j;
  LoadImbalance::LIRetriever pr;
  int annotCount = IPCGAnal::doAnnotate(c, pr, j);
  ASSERT_EQ(0, annotCount);
}

TEST(LIMetaDataTest, SimpleAnnotation) {
  nlohmann::json j;
  j["main"] = {{"numStatements", 42}, {"doesOverride", false}, {"hasBody", true}};

  auto n = std::make_shared<CgNode>("main");
  n->get<LoadImbalance::LIMetaData>()->flag(LoadImbalance::FlagType::Visited);

  Callgraph c;
  c.insert(n);

  LoadImbalance::LIRetriever pr;
  int annotCount = IPCGAnal::doAnnotate(c, pr, j);

  std::cout << j << std::endl;

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
  n->get<LoadImbalance::LIMetaData>()->flag(LoadImbalance::FlagType::Visited);

  auto n2 = std::make_shared<CgNode>("node2");
  n2->get<LoadImbalance::LIMetaData>()->flag(LoadImbalance::FlagType::Irrelevant);
  n2->get<LoadImbalance::LIMetaData>()->flag(LoadImbalance::FlagType::Visited);
  n2->get<LoadImbalance::LIMetaData>()->setNumberOfInclusiveStatements(100);
  n2->get<LoadImbalance::LIMetaData>()->setVirtual(true);

  Callgraph c;
  c.insert(n);
  c.insert(n2);

  LoadImbalance::LIRetriever pr;
  int annotCount = IPCGAnal::doAnnotate(c, pr, j);

  std::cout << j << std::endl;

  ASSERT_EQ(2, annotCount);
  ASSERT_TRUE(j["main"]["meta"]["LIData"]["visited"] == true);
  ASSERT_TRUE(j["main"]["meta"]["LIData"]["irrelevant"] == false);
  ASSERT_EQ(j["main"]["numStatements"], 42);

  ASSERT_TRUE(j["node2"]["meta"]["LIData"]["visited"] == true);
  ASSERT_TRUE(j["node2"]["meta"]["LIData"]["irrelevant"] == true);
  ASSERT_EQ(j["node2"]["numStatements"], 34);
}