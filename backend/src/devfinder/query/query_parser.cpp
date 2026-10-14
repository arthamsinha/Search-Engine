#include "devfinder/query/query_parser.hpp"
#include <sstream>

namespace devfinder {
namespace query {

ParsedQuery QueryParser::parse(const std::string& raw_query) const {
    ParsedQuery result;
    std::istringstream iss(raw_query);
    std::string token;
    
    while (iss >> token) {
        if (token.find("language:") == 0) {
            result.language_filter = token.substr(9);
        } else if (token.find("has:") == 0) {
            std::string val = token.substr(4);
            if (val == "docker") result.has_docker_filter = true;
        } else if (token.find("stars:>") == 0) {
            try {
                result.min_stars = std::stoi(token.substr(7));
            } catch (...) {}
        } else if (token.find("updated:<") == 0) {
            try {
                result.updated_within_days = std::stoi(token.substr(9));
            } catch (...) {}
        } else {
            result.keywords.push_back(token);
        }
    }
    
    return result;
}

} // namespace query
} // namespace devfinder
