#include "gtest/gtest.h"
#include <string>
#include "NodeSummary.h"
#include "NodeSummaryComparator.h"
#include "NodeSummaryHasher.h"

using std::unordered_set;


TEST(NodeTest, EqualityNoneMode) {
    NodeSummaryComparator cmp(ComparisonMode::none);

    NodeSummary foo1("foo", true, unordered_set<std::string>{"bar", "baz"});
    NodeSummary foo2("foo", true, unordered_set<std::string>{"bar", "baz"});
    NodeSummary foo3("foo", true, unordered_set<std::string>{"bar", "baz"});

    EXPECT_TRUE(cmp(foo1, foo2));
    EXPECT_TRUE(cmp(foo1, foo3));

    NodeSummary foo_hasBody("foo", true, unordered_set<std::string>{"bar", "baz"});
    NodeSummary foo_hasNoBody("foo", false, unordered_set<std::string>{"bar", "baz"});

    EXPECT_FALSE(cmp(foo_hasBody, foo_hasNoBody));

    NodeSummary foo_name1("foo", true, unordered_set<std::string>{"bar", "baz"});
    NodeSummary foo_name2("fo", true, unordered_set<std::string>{"bar", "baz"});

    EXPECT_FALSE(cmp(foo_name1, foo_name2));

    NodeSummary foo_edges1("foo", true, unordered_set<std::string>{"bar", "baz"});
    NodeSummary foo_edges2("foo", true, unordered_set<std::string>{"bar", "bax"});

    EXPECT_FALSE(cmp(foo_edges1, foo_edges2));

    std::string md1 = R"({"isTemplate":false,"likelyInline":false,"markedAlwaysInline":false,"markedInline":false})";
    std::string md1_altered = R"({"isTemplate":true,"likelyInline":false,"markedAlwaysInline":false,"markedInline":false})";
    std::string md2 = "0";

    NodeSummary foo_md1("foo", true, unordered_set<std::string>{"bar", "baz"}, unordered_set<std::string>{md1, md2});
    NodeSummary foo_md2("foo", true, unordered_set<std::string>{"bar", "baz"}, unordered_set<std::string>{md1_altered, md2});
    NodeSummary foo_md3("foo", true, unordered_set<std::string>{"bar", "baz"}, unordered_set<std::string>{md2, md1_altered});
    NodeSummary foo_md4("foo", true, unordered_set<std::string>{"bar", "baz"}, unordered_set<std::string>{md2, md1});

    EXPECT_TRUE(cmp(foo_md1, foo_md1));
    EXPECT_TRUE(cmp(foo_md1, foo_md4));
    EXPECT_TRUE(cmp(foo_md2, foo_md3));
    EXPECT_FALSE(cmp(foo_md1, foo_md2));
    EXPECT_FALSE(cmp(foo_md1, foo_md3));
    EXPECT_FALSE(cmp(foo_md3, foo_md4));
}

TEST(NodeTest, HashNoneMode) {
    NodeSummaryHasher hasher(ComparisonMode::none);

    NodeSummary foo1("foo", true, unordered_set<std::string>{"bar", "baz"});
    NodeSummary foo2("foo", true, unordered_set<std::string>{"bar", "baz"});
    NodeSummary foo3("foo", true, unordered_set<std::string>{"bar", "baz"});

    EXPECT_EQ(hasher(foo1), hasher(foo2));
    EXPECT_EQ(hasher(foo1), hasher(foo3));

    NodeSummary foo_hasBody("foo", true, unordered_set<std::string>{"bar", "baz"});
    NodeSummary foo_hasNoBody("foo", false, unordered_set<std::string>{"bar", "baz"});

    EXPECT_NE(hasher(foo_hasBody), hasher(foo_hasNoBody));

    NodeSummary foo_name1("foo", true, unordered_set<std::string>{"bar", "baz"});
    NodeSummary foo_name2("fo", true, unordered_set<std::string>{"bar", "baz"});

    EXPECT_NE(hasher(foo_name1), hasher(foo_name2));

    NodeSummary foo_edges1("foo", true, unordered_set<std::string>{"bar", "baz"});
    NodeSummary foo_edges2("foo", true, unordered_set<std::string>{"bar", "bax"});

    EXPECT_NE(hasher(foo_edges1), hasher(foo_edges2));

    std::string md1 = R"({"isTemplate":false,"likelyInline":false,"markedAlwaysInline":false,"markedInline":false})";
    std::string md1_altered = R"({"isTemplate":true,"likelyInline":false,"markedAlwaysInline":false,"markedInline":false})";
    std::string md2 = "0";

    NodeSummary foo_md1("foo", true, unordered_set<std::string>{"bar", "baz"}, unordered_set<std::string>{md1, md2});
    NodeSummary foo_md2("foo", true, unordered_set<std::string>{"bar", "baz"}, unordered_set<std::string>{md1_altered, md2});
    NodeSummary foo_md3("foo", true, unordered_set<std::string>{"bar", "baz"}, unordered_set<std::string>{md2, md1_altered});
    NodeSummary foo_md4("foo", true, unordered_set<std::string>{"bar", "baz"}, unordered_set<std::string>{md2, md1});

    EXPECT_EQ(hasher(foo_md1), hasher(foo_md1));
    EXPECT_EQ(hasher(foo_md1), hasher(foo_md4));
    EXPECT_EQ(hasher(foo_md2), hasher(foo_md3));
    EXPECT_NE(hasher(foo_md1), hasher(foo_md2));
    EXPECT_NE(hasher(foo_md1), hasher(foo_md3));
    EXPECT_NE(hasher(foo_md3), hasher(foo_md4));
}

TEST(NodeTest, EqualityIgnoreBody) {
    NodeSummaryComparator cmp_body(ComparisonMode::ignoreBody);

    NodeSummary foo1("foo", true, unordered_set<std::string>{"bar", "baz"});
    NodeSummary foo2("foo", false, unordered_set<std::string>{"bar", "baz"});

    EXPECT_TRUE(cmp_body(foo1, foo2));

    NodeSummary foo_name1("foo", true, unordered_set<std::string>{"bar", "baz"});
    NodeSummary foo_name2("fo", true, unordered_set<std::string>{"bar", "baz"});

    EXPECT_FALSE(cmp_body(foo_name1, foo_name2));

    NodeSummary foo_edges1("foo", true, unordered_set<std::string>{"bar", "baz"});
    NodeSummary foo_edges2("foo", true, unordered_set<std::string>{"bar", "bax"});

    EXPECT_FALSE(cmp_body(foo_edges1, foo_edges2));

    std::string md1 = R"({"isTemplate":false,"likelyInline":false,"markedAlwaysInline":false,"markedInline":false})";
    std::string md1_altered = R"({"isTemplate":true,"likelyInline":false,"markedAlwaysInline":false,"markedInline":false})";
    std::string md2 = "0";

    NodeSummary foo_md1("foo", true, unordered_set<std::string>{"bar", "baz"}, unordered_set<std::string>{md1, md2});
    NodeSummary foo_md2("foo", true, unordered_set<std::string>{"bar", "baz"}, unordered_set<std::string>{md1_altered, md2});
    NodeSummary foo_md3("foo", true, unordered_set<std::string>{"bar", "baz"}, unordered_set<std::string>{md2, md1_altered});
    NodeSummary foo_md4("foo", true, unordered_set<std::string>{"bar", "baz"}, unordered_set<std::string>{md2, md1});

    EXPECT_TRUE(cmp_body(foo_md1, foo_md1));
    EXPECT_TRUE(cmp_body(foo_md1, foo_md4));
    EXPECT_TRUE(cmp_body(foo_md2, foo_md3));
    EXPECT_FALSE(cmp_body(foo_md1, foo_md2));
    EXPECT_FALSE(cmp_body(foo_md1, foo_md3));
    EXPECT_FALSE(cmp_body(foo_md3, foo_md4));
}

TEST(NodeTest, HashIgnoreBody) {
    NodeSummaryHasher hasher_body(ComparisonMode::ignoreBody);

    NodeSummary foo1("foo", true, unordered_set<std::string>{"bar", "baz"});
    NodeSummary foo2("foo", false, unordered_set<std::string>{"bar", "baz"});

    EXPECT_EQ(hasher_body(foo1), hasher_body(foo2));

    NodeSummary foo_name1("foo", true, unordered_set<std::string>{"bar", "baz"});
    NodeSummary foo_name2("fo", true, unordered_set<std::string>{"bar", "baz"});

    EXPECT_NE(hasher_body(foo_name1), hasher_body(foo_name2));

    NodeSummary foo_edges1("foo", true, unordered_set<std::string>{"bar", "baz"});
    NodeSummary foo_edges2("foo", true, unordered_set<std::string>{"bar", "bax"});

    EXPECT_NE(hasher_body(foo_edges1), hasher_body(foo_edges2));

    std::string md1 = R"({"isTemplate":false,"likelyInline":false,"markedAlwaysInline":false,"markedInline":false})";
    std::string md1_altered = R"({"isTemplate":true,"likelyInline":false,"markedAlwaysInline":false,"markedInline":false})";
    std::string md2 = "0";

    NodeSummary foo_md1("foo", true, unordered_set<std::string>{"bar", "baz"}, unordered_set<std::string>{md1, md2});
    NodeSummary foo_md2("foo", true, unordered_set<std::string>{"bar", "baz"}, unordered_set<std::string>{md1_altered, md2});
    NodeSummary foo_md3("foo", true, unordered_set<std::string>{"bar", "baz"}, unordered_set<std::string>{md2, md1_altered});
    NodeSummary foo_md4("foo", true, unordered_set<std::string>{"bar", "baz"}, unordered_set<std::string>{md2, md1});

    EXPECT_EQ(hasher_body(foo_md1), hasher_body(foo_md1));
    EXPECT_EQ(hasher_body(foo_md1), hasher_body(foo_md4));
    EXPECT_EQ(hasher_body(foo_md2), hasher_body(foo_md3));
    EXPECT_NE(hasher_body(foo_md1), hasher_body(foo_md2));
    EXPECT_NE(hasher_body(foo_md1), hasher_body(foo_md3));
    EXPECT_NE(hasher_body(foo_md3), hasher_body(foo_md4));
}

TEST(NodeTest, EqualityIgnoreEdges) {
    NodeSummaryComparator cmp_edges(ComparisonMode::ignoreEdges);

    NodeSummary foo1("foo", true, unordered_set<std::string>{"bar", "baz"});
    NodeSummary foo2("foo", true, unordered_set<std::string>{"bar", "bax"});

    EXPECT_TRUE(cmp_edges(foo1, foo2));

    NodeSummary foo_hasBody("foo", true, unordered_set<std::string>{"bar", "baz"});
    NodeSummary foo_hasNoBody("foo", false, unordered_set<std::string>{"bar", "baz"});

    EXPECT_FALSE(cmp_edges(foo_hasBody, foo_hasNoBody));

    NodeSummary foo_name1("foo", true, unordered_set<std::string>{"bar", "baz"});
    NodeSummary foo_name2("fo", true, unordered_set<std::string>{"bar", "baz"});

    EXPECT_FALSE(cmp_edges(foo_name1, foo_name2));
}

TEST(NodeTest, HashIgnoreEdges) {
    NodeSummaryHasher hasher_edges(ComparisonMode::ignoreEdges);

    NodeSummary foo1("foo", true, unordered_set<std::string>{"bar", "baz"});
    NodeSummary foo2("foo", true, unordered_set<std::string>{"bar", "bax"});

    EXPECT_EQ(hasher_edges(foo1), hasher_edges(foo2));

    NodeSummary foo_hasBody("foo", true, unordered_set<std::string>{"bar", "baz"});
    NodeSummary foo_hasNoBody("foo", false, unordered_set<std::string>{"bar", "baz"});

    EXPECT_NE(hasher_edges(foo_hasBody), hasher_edges(foo_hasNoBody));

    NodeSummary foo_name1("foo", true, unordered_set<std::string>{"bar", "baz"});
    NodeSummary foo_name2("fo", true, unordered_set<std::string>{"bar", "baz"});

    EXPECT_NE(hasher_edges(foo_name1), hasher_edges(foo_name2));
}


TEST(NodeTest, EqualityIgnoreMetadata) {
    NodeSummaryComparator cmp_md(ComparisonMode::ignoreMetadata);

    std::string md1 = R"({"isTemplate":false,"likelyInline":false,"markedAlwaysInline":false,"markedInline":false})";
    std::string md1_altered = R"({"isTemplate":true,"likelyInline":false,"markedAlwaysInline":false,"markedInline":false})";
    std::string md2 = "0";

    NodeSummary foo_md1("foo", true, std::unordered_set<std::string>{"bar", "baz"},
                        std::unordered_set<std::string>{md1, md2});
    NodeSummary foo_md2("foo", true, std::unordered_set<std::string>{"bar", "baz"},
                        std::unordered_set<std::string>{md1_altered, md2});
    NodeSummary foo_md3("foo", true, std::unordered_set<std::string>{"bar", "baz"},
                        std::unordered_set<std::string>{md2, md1_altered});
    NodeSummary foo_md4("foo", true, std::unordered_set<std::string>{"bar", "baz"},
                        std::unordered_set<std::string>{md2, md1});

    EXPECT_TRUE(cmp_md(foo_md1, foo_md1));
    EXPECT_TRUE(cmp_md(foo_md1, foo_md4));
    EXPECT_TRUE(cmp_md(foo_md2, foo_md3));
    EXPECT_TRUE(cmp_md(foo_md1, foo_md2));
    EXPECT_TRUE(cmp_md(foo_md1, foo_md3));
    EXPECT_TRUE(cmp_md(foo_md3, foo_md4));

}

TEST(NodeTest, HashIgnoreMetadata) {
    NodeSummaryHasher hasher_md(ComparisonMode::ignoreMetadata);

    std::string md1 = R"({"isTemplate":false,"likelyInline":false,"markedAlwaysInline":false,"markedInline":false})";
    std::string md1_altered = R"({"isTemplate":true,"likelyInline":false,"markedAlwaysInline":false,"markedInline":false})";
    std::string md2 = "0";

    NodeSummary foo_md1("foo", true, std::unordered_set<std::string>{"bar", "baz"},
                        std::unordered_set<std::string>{md1, md2});
    NodeSummary foo_md2("foo", true, std::unordered_set<std::string>{"bar", "baz"},
                        std::unordered_set<std::string>{md1_altered, md2});
    NodeSummary foo_md3("foo", true, std::unordered_set<std::string>{"bar", "baz"},
                        std::unordered_set<std::string>{md2, md1_altered});
    NodeSummary foo_md4("foo", true, std::unordered_set<std::string>{"bar", "baz"},
                        std::unordered_set<std::string>{md2, md1});

    EXPECT_EQ(hasher_md(foo_md1), hasher_md(foo_md1));
    EXPECT_EQ(hasher_md(foo_md1), hasher_md(foo_md4));
    EXPECT_EQ(hasher_md(foo_md2), hasher_md(foo_md3));
    EXPECT_EQ(hasher_md(foo_md1), hasher_md(foo_md2));
    EXPECT_EQ(hasher_md(foo_md1), hasher_md(foo_md3));
    EXPECT_EQ(hasher_md(foo_md3), hasher_md(foo_md4));

}
