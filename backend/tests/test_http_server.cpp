#include <gtest/gtest.h>
#include "devfinder/server/http_server.hpp"
#include "devfinder/index/inverted_index.hpp"
#include "devfinder/ranking/bm25.hpp"
#include "devfinder/ranking/health_score.hpp"
#include <thread>
#include <httplib.h>
#include <nlohmann/json.hpp>

using namespace devfinder::server;
using namespace devfinder::query;
using namespace devfinder::index;
using namespace devfinder::ranking;
using namespace devfinder::autocomplete;
using namespace devfinder::common;

TEST(HttpServerTest, Endpoints) {
    InvertedIndex index;
    Bm25Ranker ranker(index);
    HealthScorer scorer;
    std::unordered_map<std::string, RepoMetadata> store;
    QueryEngine engine(index, ranker, scorer, store);
    Trie trie;
    
    HttpServer server(engine, trie);
    
    int port = 45678;
    std::thread t([&]() {
        server.listen("127.0.0.1", port);
    });
    
    std::this_thread::sleep_for(std::chrono::milliseconds(200)); // allow server to start
    
    httplib::Client cli("127.0.0.1", port);
    
    // Test Health
    auto res_health = cli.Get("/health");
    ASSERT_TRUE(res_health);
    EXPECT_EQ(res_health->status, 200);
    EXPECT_TRUE(res_health->body.find("\"status\":\"ok\"") != std::string::npos);
    
    // Test Missing q
    auto res_missing = cli.Get("/api/search");
    ASSERT_TRUE(res_missing);
    EXPECT_EQ(res_missing->status, 400);
    EXPECT_TRUE(res_missing->body.find("Missing required parameter") != std::string::npos);
    
    // Test Valid q
    auto res_valid = cli.Get("/api/search?q=test");
    ASSERT_TRUE(res_valid);
    EXPECT_EQ(res_valid->status, 200);
    EXPECT_TRUE(res_valid->body.find("\"results\":[]") != std::string::npos);
    
    server.get_raw_server().stop();
    t.join();
}
