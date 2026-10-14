#pragma once
#include <string>
#include <stdexcept>

namespace devfinder {
namespace crawler {

class RateLimitError : public std::runtime_error {
public:
    explicit RateLimitError(const std::string& msg) : std::runtime_error(msg) {}
};

class IGithubClient {
public:
    virtual ~IGithubClient() = default;
    virtual std::string get(const std::string& endpoint) = 0;
};

class GithubClient : public IGithubClient {
public:
    explicit GithubClient(std::string token);
    std::string get(const std::string& endpoint) override;
private:
    std::string token_;
};

} // namespace crawler
} // namespace devfinder
