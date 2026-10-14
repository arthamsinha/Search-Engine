#pragma once
#include "devfinder/query/query_parser.hpp"
#include "devfinder/index/inverted_index.hpp"
#include "devfinder/ranking/bm25.hpp"
#include "devfinder/ranking/health_score.hpp"
#include "devfinder/preprocessing/tokenizer.hpp"
#include <unordered_map>
#include <string>
#include <vector>

namespace devfinder {
namespace query {

class QueryEngine {
public:
    QueryEngine(index::InvertedIndex& index, ranking::Bm25Ranker& ranker, ranking::HealthScorer& scorer,
                std::unordered_map<std::string, common::RepoMetadata> repo_store);
    std::vector<ranking::ScoreBreakdown> search(const std::string& raw_query, int top_k = 10) const;

private:
    index::InvertedIndex& index_;
    ranking::Bm25Ranker& ranker_;
    ranking::HealthScorer& scorer_;
    std::unordered_map<std::string, common::RepoMetadata> repo_store_;
    preprocessing::Tokenizer tokenizer_;
};

} // namespace query
} // namespace devfinder
