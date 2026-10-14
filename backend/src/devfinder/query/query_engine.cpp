#include "devfinder/query/query_engine.hpp"
#include <algorithm>
#include <chrono>
#include <sstream>
#include <iomanip>

namespace devfinder {
namespace query {

QueryEngine::QueryEngine(index::InvertedIndex& index, ranking::Bm25Ranker& ranker, ranking::HealthScorer& scorer,
                         std::unordered_map<std::string, common::RepoMetadata> repo_store)
    : index_(index), ranker_(ranker), scorer_(scorer), repo_store_(std::move(repo_store)) {}

std::vector<ranking::ScoreBreakdown> QueryEngine::search(const std::string& raw_query, int top_k) const {
    QueryParser parser;
    ParsedQuery parsed = parser.parse(raw_query);
    
    std::vector<std::string> query_tokens;
    for (const auto& kw : parsed.keywords) {
        auto tokens = tokenizer_.tokenize(kw);
        query_tokens.insert(query_tokens.end(), tokens.begin(), tokens.end());
    }
    
    auto bm25_results = ranker_.score(query_tokens);
    std::vector<ranking::ScoreBreakdown> final_results;
    auto now = std::chrono::system_clock::now();
    
    std::unordered_map<std::string, std::vector<std::string>> doc_term_map;
    for (const auto& term : query_tokens) {
        for (const auto& p : index_.postings_for(term)) {
            doc_term_map[p.doc_id].push_back(term);
        }
    }
    
    for (const auto& [doc_id, bm25_score] : bm25_results) {
        auto it = repo_store_.find(doc_id);
        if (it == repo_store_.end()) continue;
        
        const auto& repo = it->second;
        
        if (parsed.language_filter.has_value()) {
            std::string lang = repo.primary_language;
            std::transform(lang.begin(), lang.end(), lang.begin(), ::tolower);
            std::string filter_lang = parsed.language_filter.value();
            std::transform(filter_lang.begin(), filter_lang.end(), filter_lang.begin(), ::tolower);
            if (lang != filter_lang) continue;
        }
        
        if (parsed.has_docker_filter.has_value() && parsed.has_docker_filter.value()) {
            if (!repo.has_dockerfile) continue;
        }
        
        if (parsed.min_stars.has_value()) {
            if (repo.stars < parsed.min_stars.value()) continue;
        }
        
        if (parsed.updated_within_days.has_value()) {
            if (repo.last_commit_date.empty()) continue; 
            std::tm t = {};
            std::istringstream ss(repo.last_commit_date);
            ss >> std::get_time(&t, "%Y-%m-%dT%H:%M:%S");
            if (!ss.fail()) {
                auto commit_time = std::chrono::system_clock::from_time_t(std::mktime(&t));
                if (now > commit_time) {
                    auto duration = std::chrono::duration_cast<std::chrono::hours>(now - commit_time);
                    double days = duration.count() / 24.0;
                    if (days > parsed.updated_within_days.value()) {
                        continue;
                    }
                }
            } else {
                continue;
            }
        }
        
        auto breakdown = scorer_.compute(repo, bm25_score);
        breakdown.repo_id = doc_id; 
        
        std::vector<std::string> doc_matches = doc_term_map[doc_id];
        breakdown.matched_terms = doc_matches;
        for (const auto& term : query_tokens) {
            if (std::find(doc_matches.begin(), doc_matches.end(), term) == doc_matches.end()) {
                breakdown.missing_terms.push_back(term);
            }
        }
        
        final_results.push_back(breakdown);
    }
    
    std::sort(final_results.begin(), final_results.end(), [](const auto& a, const auto& b) {
        return a.final_score > b.final_score;
    });
    
    if (static_cast<int>(final_results.size()) > top_k) {
        final_results.resize(top_k);
    }
    
    return final_results;
}

} // namespace query
} // namespace devfinder
