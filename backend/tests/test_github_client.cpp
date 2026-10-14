#include <gtest/gtest.h>
#include "devfinder/crawler/repo_fetcher.hpp"
#include "devfinder/common/models.hpp"

using namespace devfinder::crawler;
using namespace devfinder::common;

class MockGithubClient : public IGithubClient {
public:
    std::string response_to_return;
    bool throw_rate_limit = false;
    bool return_malformed = false;
    
    std::string get(const std::string& endpoint) override {
        if (throw_rate_limit) {
            throw RateLimitError("Mock rate limit");
        }
        if (return_malformed) {
            return "{ malformed json ";
        }
        if (endpoint.find("/contents/") != std::string::npos) {
            return ""; // For checking readiness files, simulate not found natively here
        }
        return response_to_return;
    }
};

TEST(GithubClientTest, SuccessfulParse) {
    auto mock = std::make_shared<MockGithubClient>();
    mock->response_to_return = R"({
        "items": [
            {
                "full_name": "test/repo",
                "name": "repo",
                "owner": {"login": "test"},
                "stargazers_count": 100,
                "topics": ["cpp"]
            }
        ]
    })";
    
    RepoFetcher fetcher(mock);
    auto repos = fetcher.fetch_by_topic("cpp", 1);
    
    ASSERT_EQ(repos.size(), 1);
    EXPECT_EQ(repos[0].id, "test/repo");
    EXPECT_EQ(repos[0].stars, 100);
    EXPECT_EQ(repos[0].topics.size(), 1);
}

TEST(GithubClientTest, RateLimitThrows) {
    auto mock = std::make_shared<MockGithubClient>();
    mock->throw_rate_limit = true;
    
    RepoFetcher fetcher(mock);
    EXPECT_THROW(fetcher.fetch_by_topic("cpp", 1), RateLimitError);
}

TEST(GithubClientTest, MalformedJsonThrows) {
    auto mock = std::make_shared<MockGithubClient>();
    mock->return_malformed = true;
    
    RepoFetcher fetcher(mock);
    EXPECT_THROW(fetcher.fetch_by_topic("cpp", 1), std::runtime_error);
}
