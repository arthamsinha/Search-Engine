#include "devfinder/preprocessing/tokenizer.hpp"
#include <unordered_set>
#include <cctype>
#include <algorithm>

namespace devfinder {
namespace preprocessing {

std::vector<std::string> Tokenizer::tokenize(const std::string& text) const {
    std::vector<std::string> result;
    std::string current_word;

    auto process_and_add = [&](const std::string& w) {
        if (w.empty()) return;
        auto subwords = split_camel_and_snake_case(w);
        for (const auto& sw : subwords) {
            std::string lower_sw = to_lowercase(sw);
            if (!lower_sw.empty() && !is_stopword(lower_sw)) {
                result.push_back(lower_sw);
            }
        }
    };

    for (char c : text) {
        if (std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '-') {
            current_word += c;
        } else {
            process_and_add(current_word);
            current_word.clear();
        }
    }
    process_and_add(current_word);

    return result;
}

std::vector<std::string> Tokenizer::split_camel_and_snake_case(const std::string& word) const {
    std::vector<std::string> parts;
    std::string current_part;

    for (size_t i = 0; i < word.length(); ++i) {
        char c = word[i];
        if (c == '_' || c == '-') {
            if (!current_part.empty()) {
                parts.push_back(current_part);
                current_part.clear();
            }
        } else if (std::isupper(static_cast<unsigned char>(c))) {
            if (!current_part.empty() && std::islower(static_cast<unsigned char>(word[i-1]))) {
                parts.push_back(current_part);
                current_part.clear();
            }
            current_part += c;
        } else {
            current_part += c;
        }
    }
    if (!current_part.empty()) {
        parts.push_back(current_part);
    }
    return parts;
}

std::string Tokenizer::to_lowercase(const std::string& word) const {
    std::string lower = word;
    std::transform(lower.begin(), lower.end(), lower.begin(),
                   [](unsigned char c){ return std::tolower(c); });
    return lower;
}

bool Tokenizer::is_stopword(const std::string& word) const {
    static const std::unordered_set<std::string> stopwords = {
        "a", "an", "and", "are", "as", "at", "be", "but", "by",
        "for", "if", "in", "into", "is", "it",
        "no", "not", "of", "on", "or", "such",
        "that", "the", "their", "then", "there", "these",
        "they", "this", "to", "was", "will", "with"
    };
    return stopwords.find(word) != stopwords.end();
}

} // namespace preprocessing
} // namespace devfinder
