#pragma once
#include "devfinder/index/inverted_index.hpp"
#include <string>
#include <vector>
#include <utility>

namespace devfinder {
namespace ranking {

class Bm25Ranker {
public:
    Bm25Ranker(const index::InvertedIndex& index, double k1 = 1.5, double b = 0.75);
    std::vector<std::pair<std::string, double>> score(const std::vector<std::string>& query_tokens) const;
private:
    const index::InvertedIndex& index_;
    double k1_;
    double b_;
    double idf(const std::string& term) const;
};

} // namespace ranking
} // namespace devfinder
