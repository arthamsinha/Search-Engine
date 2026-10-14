#include "devfinder/autocomplete/trie.hpp"
#include <cctype>
#include <queue>

namespace devfinder {
namespace autocomplete {

Trie::Trie() : root_(std::make_unique<TrieNode>()) {}
Trie::~Trie() = default;

std::string Trie::to_lowercase(const std::string& str) const {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c){ return std::tolower(c); });
    return result;
}

void Trie::insert(const std::string& word, int weight) {
    if (word.empty()) return;
    std::string lower_word = to_lowercase(word);
    
    TrieNode* current = root_.get();
    for (char c : lower_word) {
        if (current->children.find(c) == current->children.end()) {
            current->children[c] = std::make_unique<TrieNode>();
        }
        current = current->children[c].get();
    }
    
    current->is_end = true;
    current->weight += weight; 
}

void Trie::collect_suggestions(const TrieNode* node, const std::string& current_prefix, 
                               std::vector<std::pair<std::string, int>>& results) const {
    if (node->is_end) {
        results.push_back({current_prefix, node->weight});
    }
    for (const auto& [c, childNode] : node->children) {
        collect_suggestions(childNode.get(), current_prefix + c, results);
    }
}

std::vector<std::string> Trie::suggest(const std::string& prefix, int max_results) const {
    std::string lower_prefix = to_lowercase(prefix);
    
    const TrieNode* current = root_.get();
    for (char c : lower_prefix) {
        if (current->children.find(c) == current->children.end()) {
            return {}; 
        }
        current = current->children.at(c).get();
    }
    
    std::vector<std::pair<std::string, int>> raw_results;
    collect_suggestions(current, lower_prefix, raw_results);
    
    std::sort(raw_results.begin(), raw_results.end(), [](const auto& a, const auto& b) {
        return a.second > b.second;
    });
    
    std::vector<std::string> final_results;
    for (size_t i = 0; i < raw_results.size() && i < static_cast<size_t>(max_results); ++i) {
        final_results.push_back(raw_results[i].first);
    }
    
    return final_results;
}

} // namespace autocomplete
} // namespace devfinder
