#include "devfinder/index/inverted_index.hpp"
#include <stdexcept>

namespace devfinder {
namespace index {

void InvertedIndex::add_document(const std::string& doc_id, const std::vector<std::string>& tokens) {
    doc_lengths_[doc_id] = tokens.size();
    
    // Group terms by their positions in the document
    std::unordered_map<std::string, std::vector<int>> term_positions;
    for (int i = 0; i < static_cast<int>(tokens.size()); ++i) {
        term_positions[tokens[i]].push_back(i);
    }
    
    for (const auto& [term, positions] : term_positions) {
        Posting p;
        p.doc_id = doc_id;
        p.term_frequency = positions.size();
        p.positions = positions;
        
        index_[term].push_back(p);
    }
}

const std::vector<Posting>& InvertedIndex::postings_for(const std::string& term) const {
    static const std::vector<Posting> empty_postings;
    auto it = index_.find(term);
    if (it != index_.end()) {
        return it->second;
    }
    return empty_postings;
}

int InvertedIndex::document_count() const {
    return doc_lengths_.size();
}

double InvertedIndex::average_doc_length() const {
    if (doc_lengths_.empty()) return 0.0;
    
    long long total_length = 0;
    for (const auto& [_, length] : doc_lengths_) {
        total_length += length;
    }
    return static_cast<double>(total_length) / doc_lengths_.size();
}

int InvertedIndex::doc_length(const std::string& doc_id) const {
    auto it = doc_lengths_.find(doc_id);
    if (it != doc_lengths_.end()) {
        return it->second;
    }
    return 0;
}

const std::unordered_map<std::string, std::vector<Posting>>& InvertedIndex::get_raw_index() const {
    return index_;
}

const std::unordered_map<std::string, int>& InvertedIndex::get_raw_doc_lengths() const {
    return doc_lengths_;
}

void InvertedIndex::set_raw_data(std::unordered_map<std::string, std::vector<Posting>> idx,
                                 std::unordered_map<std::string, int> lengths) {
    index_ = std::move(idx);
    doc_lengths_ = std::move(lengths);
}

} // namespace index
} // namespace devfinder
