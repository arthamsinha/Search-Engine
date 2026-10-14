#pragma once
#include <string>
#include <vector>
#include <optional>

namespace devfinder {
namespace query {

struct ParsedQuery {
    std::vector<std::string> keywords;
    std::optional<std::string> language_filter;
    std::optional<bool> has_docker_filter;
    std::optional<int> min_stars;
    std::optional<int> updated_within_days;
};

class QueryParser {
public:
    ParsedQuery parse(const std::string& raw_query) const;
};

} // namespace query
} // namespace devfinder
