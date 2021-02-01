#include "gtest/gtest.h"
#include <loadImbalance/LIRetriever.h>

#include "libIPCG/IPCGReader.h"

using namespace pira;

  struct DummyRetriever {
    bool handles(const CgNodePtr n) {
      return true;
    }

    int value(const CgNodePtr n) {
      return 42;
    }

    std::string toolName() {
      return "test";
    }
  };

TEST(IPCGAnnotation, EmptyCG) {
  Callgraph c;
  nlohmann::json j;
  DummyRetriever pr;
  int annotCount = IPCGAnal::doAnnotate(c, pr, j);
  ASSERT_EQ(0, annotCount);
}

TEST(IPCGAnnotation, HandlesOneNode) {
  Callgraph c;
  auto n = std::make_shared<CgNode>("main");
  c.insert(n);
  nlohmann::json j;
    j["main"] = {
      {"numStatements", 42},
      {"doesOverride", false},
      {"hasBody", true},
      {"meta", {}}
    };
    j["foo"] = {
      {"numStatements", 42},
      {"doesOverride", false},
      {"hasBody", true}
    };
  DummyRetriever pr;
  int annotCount = IPCGAnal::doAnnotate(c, pr, j);
  ASSERT_EQ(1, annotCount);
  nlohmann::json j2;
    j2["main"] = {
      {"numStatements", 42},
      {"doesOverride", false},
      {"hasBody", true},
    };
    j2["main"]["meta"]["test"] = 42;
    j2["foo"] = {
      {"numStatements", 42},
      {"doesOverride", false},
      {"hasBody", true}
    };
  ASSERT_TRUE(j == j2);
}

TEST(IPCGAnnotation, HandlesTwoNodes) {
  Callgraph c;
  auto n = std::make_shared<CgNode>("main");
  c.insert(n);
  n = std::make_shared<CgNode>("foo");
  c.insert(n);
  nlohmann::json j;
    j["main"] = {
      {"numStatements", 42},
      {"doesOverride", false},
      {"hasBody", true},
      {"meta", {}}
    };
    j["foo"] = {
      {"numStatements", 42},
      {"doesOverride", false},
      {"hasBody", true}
    };

  DummyRetriever pr;
  int annotCount = IPCGAnal::doAnnotate(c, pr, j);
  ASSERT_EQ(2, annotCount);
  nlohmann::json j2;
    j2["main"] = {
      {"numStatements", 42},
      {"doesOverride", false},
      {"hasBody", true},
    };
    j2["main"]["meta"]["test"] = 42;
    j2["foo"] = {
      {"numStatements", 42},
      {"doesOverride", false},
      {"hasBody", true}
    };
    j2["foo"]["meta"]["test"] = 42;
  ASSERT_TRUE(j == j2);
}

TEST(IPCGAnnotation, AttachesRuntime) {
  Callgraph c;
  auto n = std::make_shared<CgNode>("main");
  n->get<BaseProfileData>()->setRuntimeInSeconds(1.2);
  c.insert(n);
  n = std::make_shared<CgNode>("foo");
  n->get<BaseProfileData>()->setRuntimeInSeconds(0.0);
  c.insert(n);
  nlohmann::json j;
    j["main"] = {
      {"numStatements", 42},
      {"doesOverride", false},
      {"hasBody", true},
      {"meta", {}}
    };
    j["foo"] = {
      {"numStatements", 42},
      {"doesOverride", false},
      {"hasBody", true}
    };

  IPCGAnal::retriever::RuntimeRetriever pr;
  int annotCount = IPCGAnal::doAnnotate(c, pr, j);
  ASSERT_EQ(1, annotCount);
  nlohmann::json j2;
    j2["main"] = {
      {"numStatements", 42},
      {"doesOverride", false},
      {"hasBody", true},
    };
    j2["main"]["meta"]["test"] = 1.2;
    j2["foo"] = {
      {"numStatements", 42},
      {"doesOverride", false},
      {"hasBody", true}
    };
  ASSERT_TRUE(j == j2);
}

TEST(IPCGAnnotation, PlacementOutput) {
  Callgraph c;
  auto n = std::make_shared<CgNode>("main");
  n->get<BaseProfileData>()->setRuntimeInSeconds(1.2);
  c.insert(n);
  nlohmann::json j;
    j["main"] = {
      {"numStatements", 42},
      {"doesOverride", false},
      {"hasBody", true}
    };
  // we filter for attached model, thus do not annotate
  IPCGAnal::retriever::PlacementInfoRetriever pr;
  int annotCount = IPCGAnal::doAnnotate(c, pr, j);
  std::cout << j << std::endl;
  ASSERT_EQ(0, annotCount);
}