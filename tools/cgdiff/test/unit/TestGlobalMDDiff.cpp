#include <gtest/gtest.h>
#include "GlobalMDDiff.h"
#include "Diff.h"

TEST(GlobalMDDiff, ConstructorSetsAllFields) {
    std::string key = "entryFunction";
    std::string aValue = "thisIsMain";
    std::string bValue = "thisIsNotMain";

    GlobalMDDiff diff(key, aValue, bValue);

    EXPECT_EQ(diff.key, key);
    EXPECT_EQ(diff.aValue, aValue);
    EXPECT_EQ(diff.bValue, bValue);
}

TEST(GlobalMDDiff, KindReturnsGlobalMD) {
    GlobalMDDiff diff("k", "a", "b");
    EXPECT_EQ(diff.kind(), Diff::Kind::GlobalMD);
}

TEST(GlobalMDDiff, ToJsonContainsCorrectFields) {
    GlobalMDDiff diff("entryFunction", "thisIsMain", "thisIsNotMain");
    auto j = diff.toJson();

    EXPECT_TRUE(j.contains("aValue"));
    EXPECT_TRUE(j.contains("bValue"));

    EXPECT_EQ(j["aValue"], "thisIsMain");
    EXPECT_EQ(j["bValue"], "thisIsNotMain");
}
