#pragma once
#include <string>
#include <unordered_map>

namespace devfinder {
namespace common {

class Config {
public:
    static Config load_from_env(const std::string& env_path = ".env");
    
    std::string github_token() const;
    std::string data_dir() const;
    int server_port() const;

private:
    std::unordered_map<std::string, std::string> values_;
};

} // namespace common
} // namespace devfinder
