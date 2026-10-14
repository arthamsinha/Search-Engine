#pragma once
#include <string>
#include <vector>

namespace devfinder {
namespace index {

struct Posting {
    std::string doc_id;
    int term_frequency = 0;
    std::vector<int> positions;

    // For ease of testing
    bool operator==(const Posting& other) const {
        return doc_id == other.doc_id && 
               term_frequency == other.term_frequency &&
               positions == other.positions;
    }
};

} // namespace index
} // namespace devfinder
