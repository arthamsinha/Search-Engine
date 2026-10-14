#include <iostream>
#include <fstream>
#include <filesystem>
#include <iomanip>
#include "devfinder/common/config.hpp"
#include "devfinder/common/models.hpp"
#include "devfinder/crawler/github_client.hpp"
#include "devfinder/crawler/repo_fetcher.hpp"
#include "devfinder/preprocessing/tokenizer.hpp"
#include "devfinder/index/inverted_index.hpp"
#include "devfinder/index/index_storage.hpp"
#include "devfinder/ranking/bm25.hpp"
#include "devfinder/ranking/health_score.hpp"
#include "devfinder/query/query_engine.hpp"
#include "devfinder/autocomplete/trie.hpp"
#include "devfinder/server/http_server.hpp"
#include <nlohmann/json.hpp>

using namespace devfinder;
namespace fs = std::filesystem;

// Writes via a temp file + rename so an interrupted run never leaves a truncated/corrupt repo file.
static void save_repo_atomic(const std::string& raw_dir, const common::RepoMetadata& r) {
    nlohmann::json j = r;
    std::string final_path = raw_dir + r.owner + "_" + r.name + ".json";
    std::string tmp_path = final_path + ".tmp";
    {
        std::ofstream out(tmp_path, std::ios::trunc);
        out << j.dump(4);
        if (!out) throw std::runtime_error("Failed to write " + tmp_path);
    }
    std::error_code ec;
    fs::rename(tmp_path, final_path, ec);
    if (ec) {
        fs::copy_file(tmp_path, final_path, fs::copy_options::overwrite_existing, ec);
        fs::remove(tmp_path);
        if (ec) throw std::runtime_error("Failed to save " + final_path + ": " + ec.message());
    }
}

// Collects ids of repos already on disk so repeated runs accumulate instead of duplicating.
static std::unordered_set<std::string> load_known_ids(const std::string& raw_dir) {
    std::unordered_set<std::string> ids;
    if (!fs::exists(raw_dir)) return ids;
    for (const auto& entry : fs::directory_iterator(raw_dir)) {
        if (entry.path().extension() != ".json") continue;
        try {
            std::ifstream in(entry.path());
            nlohmann::json j;
            in >> j;
            std::string id = j.value("id", "");
            if (!id.empty()) ids.insert(id);
        } catch (const std::exception&) {
            // unreadable file: ignore, it will simply be re-fetched/overwritten if seen again
        }
    }
    return ids;
}

void print_usage(const char* prog_name) {
    std::cerr << "Usage:\n"
              << "  " << prog_name << " fetch <topic> [max_results]\n"
              << "  " << prog_name << " fetch-all [max_per_topic] [seed_file]\n"
              << "  " << prog_name << " index\n"
              << "  " << prog_name << " search \"<query>\"\n"
              << "  " << prog_name << " serve [port]\n";
}

int run_internal(int argc, char* argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }
    
    std::string command = argv[1];
    
    try {
        auto config = common::Config::load_from_env(".env");
        std::string raw_dir = config.data_dir() + "raw/";
        std::string index_path = config.data_dir() + "index.bin";
        std::string trie_path = config.data_dir() + "trie_terms.txt";
        
        if (command == "fetch") {
            if (argc < 3) {
                std::cerr << "Missing <topic> for fetch.\n";
                return 1;
            }
            std::string topic = argv[2];
            int max_results = (argc >= 4) ? std::stoi(argv[3]) : 20;
            
            auto client = std::make_shared<crawler::GithubClient>(config.github_token());
            crawler::RepoFetcher fetcher(client);
            
            std::cout << "Fetching " << max_results << " repos for topic: " << topic << "...\n";
            auto repos = fetcher.fetch_by_topic(topic, max_results);
            
            fs::create_directories(raw_dir);
            for (const auto& r : repos) save_repo_atomic(raw_dir, r);
            std::cout << "Successfully fetched and saved " << repos.size() << " repositories to " << raw_dir << "\n";
            
        } else if (command == "fetch-all") {
            int max_per_topic = (argc >= 3) ? std::stoi(argv[2]) : 20;
            std::string seed_path = (argc >= 4) ? argv[3] : "backend/config/seed_topics.txt";
            auto topics = crawler::RepoFetcher::load_seed_topics(seed_path);
            if (topics.empty()) {
                std::cerr << "No topics found in " << seed_path << "\n";
                return 1;
            }

            auto client = std::make_shared<crawler::GithubClient>(config.github_token());
            crawler::RepoFetcher fetcher(client);
            const bool has_token = !config.github_token().empty();
            // Search API: 30 req/min with a token, 10 req/min without.
            fetcher.set_request_delay_ms(has_token ? 2100 : 6500);
            fetcher.set_rate_limit_backoff_seconds(65);
            // Dockerfile/CI detection costs 2 core-API calls per repo; unauthenticated quota is 60/hour.
            fetcher.set_enrich_devops(has_token);
            if (!has_token) {
                std::cout << "Note: GITHUB_TOKEN is empty - requests are slower and Dockerfile/CI detection is skipped.\n";
            }

            fs::create_directories(raw_dir);
            auto known_ids = load_known_ids(raw_dir);
            std::cout << "Seed list: " << topics.size() << " topics from " << seed_path
                      << " | " << known_ids.size() << " repos already on disk\n";

            auto summary = fetcher.fetch_topics(topics, max_per_topic, known_ids,
                [&](const std::vector<common::RepoMetadata>& batch) {
                    for (const auto& r : batch) save_repo_atomic(raw_dir, r);
                });

            std::cout << "\nIngestion " << (summary.aborted ? "ABORTED EARLY" : "complete") << ": "
                      << summary.topics_succeeded << " topics ok, "
                      << summary.topics_empty << " empty, "
                      << summary.topics_failed << " failed; "
                      << summary.new_repos << " new repos saved, "
                      << summary.duplicate_repos << " duplicates skipped ("
                      << known_ids.size() << " total on disk).\n";
            if (summary.aborted) {
                std::cout << "Already-fetched data was kept. Re-run fetch-all to resume (known repos are skipped).\n";
            }

        } else if (command == "index") {
            std::cout << "Building inverted index from " << raw_dir << "...\n";
            index::InvertedIndex index;
            preprocessing::Tokenizer tokenizer;
            
            std::unordered_map<std::string, int> trie_terms;
            int docs_processed = 0;
            
            if (fs::exists(raw_dir)) {
                for (const auto& entry : fs::directory_iterator(raw_dir)) {
                    if (entry.path().extension() == ".json") {
                        std::ifstream in(entry.path());
                        nlohmann::json j;
                        in >> j;
                        auto repo = j.get<common::RepoMetadata>();
                        
                        std::string full_text = repo.name + " " + repo.description;
                        for (const auto& t : repo.topics) full_text += " " + t;
                        
                        auto tokens = tokenizer.tokenize(full_text);
                        index.add_document(repo.id, tokens);
                        docs_processed++;
                        
                        for (const auto& t : repo.topics) trie_terms[t]++;
                        if (!repo.primary_language.empty()) {
                            trie_terms[repo.primary_language]++;
                        }
                        auto name_tokens = tokenizer.tokenize(repo.name);
                        for (const auto& t : name_tokens) trie_terms[t]++;
                    }
                }
            }
            
            fs::create_directories(config.data_dir());
            index::IndexStorage::save(index, index_path);
            
            std::ofstream trie_out(trie_path);
            for (const auto& [term, freq] : trie_terms) {
                if (!term.empty()) trie_out << term << " " << freq << "\n";
            }
            
            std::cout << "Indexed " << docs_processed << " documents successfully to " << index_path << ".\n";
            std::cout << "Saved " << trie_terms.size() << " trie terms to " << trie_path << ".\n";
            
        } else if (command == "search" || command == "serve") {
            if (!fs::exists(index_path)) {
                std::cerr << "Index not found at " << index_path << ". Please run 'devfinder index' first.\n";
                return 1;
            }
            index::InvertedIndex index = index::IndexStorage::load(index_path);
            
            std::unordered_map<std::string, common::RepoMetadata> repo_store;
            if (fs::exists(raw_dir)) {
                for (const auto& entry : fs::directory_iterator(raw_dir)) {
                    if (entry.path().extension() == ".json") {
                        std::ifstream in(entry.path());
                        nlohmann::json j;
                        in >> j;
                        auto repo = j.get<common::RepoMetadata>();
                        repo_store[repo.id] = repo;
                    }
                }
            }
            
            ranking::Bm25Ranker ranker(index);
            ranking::HealthScorer scorer;
            query::QueryEngine engine(index, ranker, scorer, repo_store);
            
            autocomplete::Trie trie;
            if (fs::exists(trie_path)) {
                std::ifstream trie_in(trie_path);
                std::string term;
                int freq;
                while (trie_in >> term >> freq) {
                    trie.insert(term, freq);
                }
            }
            
            if (command == "search") {
                if (argc < 3) {
                    std::cerr << "Missing <query> for search.\n";
                    return 1;
                }
                std::string raw_query = argv[2];
                auto results = engine.search(raw_query);
                
                std::cout << "Search results for: \"" << raw_query << "\"\n";
                std::cout << std::string(80, '-') << "\n";
                
                if (results.empty()) {
                    std::cout << "No matching repositories found.\n";
                } else {
                    int rank = 1;
                    for (const auto& res : results) {
                        std::cout << rank++ << ". " << res.repo_id << "\n";
                        std::cout << std::fixed << std::setprecision(4);
                        std::cout << "   Final Score: " << res.final_score << "\n";
                        std::cout << "   [Breakdown] Relevance (BM25): " << res.relevance_score << "\n";
                        std::cout << "               Popularity:       " << res.popularity_score << "\n";
                        std::cout << "               Maintenance:      " << res.maintenance_score << "\n";
                        std::cout << "               DevOps:           " << res.devops_score << "\n";
                        std::cout << std::string(80, '-') << "\n";
                    }
                }
            } else if (command == "serve") {
                int port = (argc >= 3) ? std::stoi(argv[2]) : 45678;
                server::HttpServer http_server(engine, trie);
                std::cout << "Starting HTTP server on port " << port << "...\n";
                http_server.listen("0.0.0.0", port);
            }
        } else {
            std::cerr << "Unknown command: " << command << "\n";
            print_usage(argv[0]);
            return 1;
        }
    } catch (const std::exception& e) {
        std::cerr << "Fatal Error: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}

#ifdef _WIN32
#define DEVFINDER_EXPORT __declspec(dllexport)
#else
#define DEVFINDER_EXPORT __attribute__((visibility("default")))
#endif

extern "C" DEVFINDER_EXPORT int RunCommand(const char* cmd, const char* arg1, const char* arg2) {
    char* argv[4];
    int argc = 1;
    argv[0] = const_cast<char*>("devfinder_cli");
    
    if (cmd && cmd[0] != '\0') argv[argc++] = const_cast<char*>(cmd);
    if (arg1 && arg1[0] != '\0') argv[argc++] = const_cast<char*>(arg1);
    if (arg2 && arg2[0] != '\0') argv[argc++] = const_cast<char*>(arg2);

    return run_internal(argc, argv);
}

int main(int argc, char* argv[]) {
    return run_internal(argc, argv);
}
