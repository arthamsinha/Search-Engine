#include <gtest/gtest.h>
#include "devfinder/query/query_parser.hpp"
#include "devfinder/query/query_engine.hpp"
#include "devfinder/ranking/health_score.hpp"
#include "devfinder/ranking/bm25.hpp"
#include "devfinder/index/inverted_index.hpp"

using namespace devfinder::query;
using namespace devfinder::index;
using namespace devfinder::ranking;
using namespace devfinder::common;

TEST(QueryParserTest, MultiFilterQuery) {
    QueryParser parser;
    auto p = parser.parse("kubernetes operator language:go has:docker stars:>1000 updated:<180");
    
    ASSERT_EQ(p.keywords.size(), 2);
    EXPECT_EQ(p.keywords[0], "kubernetes");
    EXPECT_EQ(p.keywords[1], "operator");
    EXPECT_EQ(p.language_filter.value_or(""), "go");
    EXPECT_TRUE(p.has_docker_filter.value_or(false));
    EXPECT_EQ(p.min_stars.value_or(0), 1000);
    EXPECT_EQ(p.updated_within_days.value_or(0), 180);
}

TEST(QueryParserTest, NoFilters) {
    QueryParser parser;
    auto p = parser.parse("just some words");
    
    ASSERT_EQ(p.keywords.size(), 3);
    EXPECT_FALSE(p.language_filter.has_value());
    EXPECT_FALSE(p.has_docker_filter.has_value());
    EXPECT_FALSE(p.min_stars.has_value());
    EXPECT_FALSE(p.updated_within_days.has_value());
}

TEST(QueryParserTest, MalformedFilterSyntaxDropped) {
    QueryParser parser;
    auto p = parser.parse("stars:>abc updated:<xyz");
    
    EXPECT_FALSE(p.min_stars.has_value());
    EXPECT_FALSE(p.updated_within_days.has_value());
}

TEST(QueryEngineTest, EndToEndIntegration) {
    InvertedIndex index;
    index.add_document("owner/repo1", {"kubernetes", "operator"});
    index.add_document("owner/repo2", {"kubernetes", "operator"}); 
    
    Bm25Ranker ranker(index);
    HealthScorer scorer;
    
    std::unordered_map<std::string, RepoMetadata> store;
    
    RepoMetadata r1;
    r1.id = "owner/repo1";
    r1.stars = 1500;
    r1.primary_language = "go";
    r1.has_dockerfile = true;
    store["owner/repo1"] = r1;
    
    RepoMetadata r2;
    r2.id = "owner/repo2";
    r2.stars = 2000;
    r2.primary_language = "python";
    r2.has_dockerfile = true;
    store["owner/repo2"] = r2;
    
    QueryEngine engine(index, ranker, scorer, store);
    
    auto results = engine.search("kubernetes language:go");
    
    ASSERT_EQ(results.size(), 1);
    EXPECT_EQ(results[0].repo_id, "owner/repo1");
}
