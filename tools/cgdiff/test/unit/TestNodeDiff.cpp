#include "gtest/gtest.h"
#include <string>
#include "NodeDiff.h"
#include "NodeSummary.h"


TEST(NodeSummary, OnlyInA_ProducesCorrectJson) {
    NodeSummary ns("foo", true, {"callee1"}, {"meta1"});

    NodeDiff diff = NodeDiff::onlyInA(ns);

    EXPECT_EQ(diff.name, "foo");
    EXPECT_TRUE(diff.diffType.size() == 1);
    EXPECT_EQ(diff.diffType[0], "missingNode");
    EXPECT_TRUE(diff.edgeDiffs.empty());
    EXPECT_TRUE(diff.metadataOnlyInA.empty());
    EXPECT_TRUE(diff.metadataOnlyInB.empty());
    EXPECT_EQ(diff.kind(), metacg::Diff::Kind::Node);
    EXPECT_EQ(diff.in, "cgA");

}

TEST(NodeDiff, OnlyInB_SetsMissingNodeCorrectly) {
    NodeSummary ns("bar", false, {"callee2"}, {"meta2"});

    NodeDiff diff = NodeDiff::onlyInB(ns);

    EXPECT_EQ(diff.name, "bar");
    EXPECT_TRUE(diff.diffType.size() == 1);
    EXPECT_EQ(diff.diffType[0], "missingNode");
    EXPECT_TRUE(diff.edgeDiffs.empty());
    EXPECT_TRUE(diff.metadataOnlyInA.empty());
    EXPECT_TRUE(diff.metadataOnlyInB.empty());
    EXPECT_EQ(diff.kind(), metacg::Diff::Kind::Node);
    EXPECT_EQ(diff.in, "cgB");
}

TEST(NodeDiff, ToJsonContainsAllExpectedKeys) {
    NodeDiff::EdgeDiff ed;
    ed.callee = "callee1";
    ed.onlyInA = true;
    ed.metadataOnlyInA = {"m1"};

    NodeDiff diff("foo",
                  {"changedMetadata"},
                  {ed},
                  {"metaA"},
                  {"metaB"});

    auto j = diff.toJson();

    // Top-level keys
    EXPECT_TRUE(j.contains("diffType"));
    EXPECT_TRUE(j.contains("metadataOnlyInA"));
    EXPECT_TRUE(j.contains("metadataOnlyInB"));
    EXPECT_TRUE(j.contains("edges"));

    // Check values
    EXPECT_EQ(j["diffType"].size(), 1);
    EXPECT_EQ(j["diffType"][0], "changedMetadata");

    EXPECT_EQ(j["metadataOnlyInA"].size(), 1);
    EXPECT_EQ(j["metadataOnlyInA"][0], "metaA");

    EXPECT_EQ(j["metadataOnlyInB"].size(), 1);
    EXPECT_EQ(j["metadataOnlyInB"][0], "metaB");

    // Check edge info
    ASSERT_TRUE(j["edges"].contains("callee1"));
    auto edge = j["edges"]["callee1"];

    EXPECT_EQ(edge["in"], "cgA"); // weil onlyInA = true
    EXPECT_EQ(edge["metadataOnlyInA"].size(), 1);
    EXPECT_EQ(edge["metadataOnlyInA"][0], "m1");
    EXPECT_TRUE(edge["metadataOnlyInB"].empty());
}
