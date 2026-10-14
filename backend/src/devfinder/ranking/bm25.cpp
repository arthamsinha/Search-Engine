#include "devfinder/ranking/bm25.hpp"
#include <cmath>
#include <unordered_map>
#include <algorithm>

namespace devfinder {
namespace ranking {

Bm25Ranker::Bm25Ranker(const index::InvertedIndex& index, double k1, double b)
    : index_(index), k1_(k1), b_(b) {}

double Bm25Ranker::idf(const std::string& term) const {
    double N = static_cast<double>(index_.document_count());
    if (N == 0) return 0.0;
    
    double n = static_cast<double>(index_.postings_for(term).size());
    // Safe IDF formula to prevent negative values
    return std::log((N - n + 0.5) / (n + 0.5) + 1.0);
}

std::vector<std::pair<std::string, double>> Bm25Ranker::score(const std::vector<std::string>& query_tokens) const {
    std::unordered_map<std::string, double> doc_scores;
    std::unordered_map<std::string, int> doc_match_counts;
    
    double avgdl = index_.average_doc_length();
    if (avgdl == 0.0) return {};

    for (const auto& term : query_tokens) {
        double current_idf = idf(term);
        if (current_idf <= 0.0) continue; 
        
        const auto& postings = index_.postings_for(term);
        for (const auto& p : postings) {
            double f = static_cast<double>(p.term_frequency);
            double dl = static_cast<double>(index_.doc_length(p.doc_id));
            
            double numerator = f * (k1_ + 1.0);
            double denominator = f + k1_ * (1.0 - b_ + b_ * (dl / avgdl));
            
            doc_scores[p.doc_id] += current_idf * (numerator / denominator);
            doc_match_counts[p.doc_id]++;
        }
    }
    
    int min_matches = static_cast<int>(std::ceil(query_tokens.size() * 0.5));
    std::vector<std::pair<std::string, double>> results;
    for (const auto& [doc_id, score] : doc_scores) {
        if (doc_match_counts[doc_id] >= min_matches) {
            results.push_back({doc_id, score});
        }
    }
    std::sort(results.begin(), results.end(), [](const auto& a, const auto& b) {
        return a.second > b.second; // Descending
    });
    
    return results;
}

} // namespace ranking
} // namespace devfinder
