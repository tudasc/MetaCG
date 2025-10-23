#include "gtest/gtest.h"
#include <string>
#include "NodeDiff.h"
#include "NodeSummary.h"


TEST(NodeSummary, ConstructorSetsAllFields) {
    std::unordered_set<std::string> callees = {"foo", "bar"};
    std::unordered_set<std::string> metadata = {"meta1", "meta2"};
    std::unordered_map<std::string, std::unordered_set<std::string>> edgeMeta = {
        {"foo", {"edgeMeta1"}}
    };

    NodeSummary ns("myFunc", true, callees, metadata, edgeMeta);

    EXPECT_EQ(ns.name, "myFunc");
    EXPECT_TRUE(ns.hasBody);
    EXPECT_EQ(ns.callees, callees);
    EXPECT_EQ(ns.metadata, metadata);
    EXPECT_EQ(ns.edgeMetadata.at("foo"), std::unordered_set<std::string>({"edgeMeta1"}));
}

TEST(NodeSummary, FullConstructorSetsAllFields) {
    std::unordered_set<std::string> callees = {"foo", "bar"};
    std::unordered_set<std::string> metadata = {"meta1", "meta2"};
    std::unordered_map<std::string, std::unordered_set<std::string>> edgeMeta = {
        {"foo", {"edgeMeta1"}},
        {"bar", {"edgeMeta2"}}
    };

    NodeSummary ns("myFunc", true, callees, metadata, edgeMeta);

    EXPECT_EQ(ns.name, "myFunc");
    EXPECT_TRUE(ns.hasBody);

    EXPECT_EQ(ns.callees, callees);
    EXPECT_EQ(ns.metadata, metadata);

    ASSERT_TRUE(ns.edgeMetadata.count("foo"));
    EXPECT_EQ(ns.edgeMetadata.at("foo"),
              std::unordered_set<std::string>({"edgeMeta1"}));
    ASSERT_TRUE(ns.edgeMetadata.count("bar"));
    EXPECT_EQ(ns.edgeMetadata.at("bar"),
              std::unordered_set<std::string>({"edgeMeta2"}));
}

TEST(NodeSummary, MinimalConstructorLeavesOthersEmpty) {
    std::unordered_set<std::string> callees = {"foo"};
    NodeSummary ns("otherFunc", false, callees);

    EXPECT_EQ(ns.name, "otherFunc");
    EXPECT_FALSE(ns.hasBody);
    EXPECT_EQ(ns.callees, callees);

    // Rest sollte leer sein
    EXPECT_TRUE(ns.metadata.empty());
    EXPECT_TRUE(ns.edgeMetadata.empty());
}

TEST(NodeSummary, LValAndRValBothWork) {
    // check template S&& works with std::string and const char*
    NodeSummary ns1("literalName", true);
    NodeSummary ns2(std::string("stdStringName"), true);

    EXPECT_EQ(ns1.name, "literalName");
    EXPECT_EQ(ns2.name, "stdStringName");
}
