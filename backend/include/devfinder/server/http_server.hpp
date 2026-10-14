#pragma once
#include "devfinder/query/query_engine.hpp"
#include "devfinder/autocomplete/trie.hpp"
#include <httplib.h>
#include <memory>
#include <string>

namespace devfinder {
namespace server {

class HttpServer {
public:
    HttpServer(const query::QueryEngine& engine, const autocomplete::Trie& trie);
    
    void listen(const std::string& host, int port);
    
    httplib::Server& get_raw_server();

private:
    const query::QueryEngine& engine_;
    const autocomplete::Trie& trie_;
    httplib::Server svr_;
    
    void setup_routes();
};

} // namespace server
} // namespace devfinder
