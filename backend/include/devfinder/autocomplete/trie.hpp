#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <algorithm>

namespace devfinder {
namespace autocomplete {

class Trie {
public:
    Trie();
    ~Trie();
    void insert(const std::string& word, int weight = 1);
    std::vector<std::string> suggest(const std::string& prefix, int max_results = 5) const;

private:
    struct TrieNode {
        std::unordered_map<char, std::unique_ptr<TrieNode>> children;
        bool is_end = false;
        int weight = 0;
    };
    std::unique_ptr<TrieNode> root_;

    void collect_suggestions(const TrieNode* node, const std::string& current_prefix, 
                             std::vector<std::pair<std::string, int>>& results) const;
    std::string to_lowercase(const std::string& str) const;
};

} // namespace autocomplete
} // namespace devfinder
