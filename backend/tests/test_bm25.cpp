#include <gtest/gtest.h>
#include "devfinder/ranking/bm25.hpp"
#include "devfinder/index/inverted_index.hpp"
#include <cmath>

using namespace devfinder::ranking;
using namespace devfinder::index;

TEST(Bm25Test, ScoreMatchesHandCalculated) {
    InvertedIndex idx;
    idx.add_document("doc1", {"hello", "world"});
    idx.add_document("doc2", {"hello", "hello"});
    
    Bm25Ranker ranker(idx, 1.5, 0.75);
    
    auto results = ranker.score({"world"});
    ASSERT_EQ(results.size(), 1);
    EXPECT_EQ(results[0].first, "doc1");
    // Hand calculated ln((2 - 1 + 0.5) / (1 + 0.5) + 1.0) = ln(2.0) = 0.693147
    EXPECT_NEAR(results[0].second, 0.69314718056, 1e-5);
}

TEST(Bm25Test, HigherFrequencyScoresHigher) {
    InvertedIndex idx;
    idx.add_document("doc1", {"hello", "world"});
    idx.add_document("doc2", {"hello", "hello"});
    
    Bm25Ranker ranker(idx, 1.5, 0.75);
    
    auto results = ranker.score({"hello"});
    ASSERT_EQ(results.size(), 2);
    // doc2 has higher frequency for "hello" than doc1, same doc length
    EXPECT_EQ(results[0].first, "doc2");
    EXPECT_EQ(results[1].first, "doc1");
    EXPECT_GT(results[0].second, results[1].second);
}

TEST(Bm25Test, AbsentTermEmptyResult) {
    InvertedIndex idx;
    idx.add_document("doc1", {"hello", "world"});
    
    Bm25Ranker ranker(idx, 1.5, 0.75);
    auto results = ranker.score({"missing"});
    EXPECT_TRUE(results.empty());
}
