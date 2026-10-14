#include "devfinder/common/config.hpp"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <cctype>

namespace devfinder {
namespace common {

Config Config::load_from_env(const std::string& env_path) {
    Config config;
    std::ifstream file(env_path);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open environment file: " + env_path);
    }

    std::string line;
    while (std::getline(file, line)) {
        // Basic trim
        line.erase(line.begin(), std::find_if(line.begin(), line.end(), [](unsigned char ch) {
            return !std::isspace(ch);
        }));
        
        if (line.empty() || line[0] == '#') {
            continue;
        }

        auto pos = line.find('=');
        if (pos != std::string::npos) {
            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);
            config.values_[key] = value;
        }
    }
    return config;
}

std::string Config::github_token() const {
    auto it = values_.find("GITHUB_TOKEN");
    if (it != values_.end()) {
        return it->second;
    }
    return "";
}

std::string Config::data_dir() const {
    auto it = values_.find("DATA_DIR");
    if (it != values_.end()) {
        return it->second;
    }
    return "backend/data/";
}

int Config::server_port() const {
    auto it = values_.find("SERVER_PORT");
    if (it != values_.end()) {
        try {
            return std::stoi(it->second);
        } catch (...) {
            return 8080;
        }
    }
    return 8080; // default
}

} // namespace common
} // namespace devfinder
