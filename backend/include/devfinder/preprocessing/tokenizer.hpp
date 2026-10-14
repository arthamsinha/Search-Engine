#pragma once
#include <string>
#include <vector>

namespace devfinder {
namespace preprocessing {

class Tokenizer {
public:
    std::vector<std::string> tokenize(const std::string& text) const;
private:
    std::vector<std::string> split_camel_and_snake_case(const std::string& word) const;
    std::string to_lowercase(const std::string& word) const;
    bool is_stopword(const std::string& word) const;
};

} // namespace preprocessing
} // namespace devfinder
