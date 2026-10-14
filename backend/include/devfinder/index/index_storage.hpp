#pragma once
#include "devfinder/index/inverted_index.hpp"
#include <string>

namespace devfinder {
namespace index {

class IndexStorage {
public:
    static void save(const InvertedIndex& index, const std::string& path);
    static InvertedIndex load(const std::string& path);
};

} // namespace index
} // namespace devfinder
