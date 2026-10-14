#pragma once
#include "devfinder/index/posting_list.hpp"
#include <unordered_map>
#include <string>
#include <vector>

namespace devfinder {
namespace index {

class InvertedIndex {
public:
    void add_document(const std::string& doc_id, const std::vector<std::string>& tokens);
    const std::vector<Posting>& postings_for(const std::string& term) const;
    int document_count() const;
    double average_doc_length() const;
    int doc_length(const std::string& doc_id) const;

    // For IndexStorage
    const std::unordered_map<std::string, std::vector<Posting>>& get_raw_index() const;
    const std::unordered_map<std::string, int>& get_raw_doc_lengths() const;
    
    // To allow IndexStorage to construct it
    void set_raw_data(std::unordered_map<std::string, std::vector<Posting>> idx,
                      std::unordered_map<std::string, int> lengths);

private:
    std::unordered_map<std::string, std::vector<Posting>> index_;
    std::unordered_map<std::string, int> doc_lengths_;
};

} // namespace index
} // namespace devfinder
