#include "devfinder/crawler/repo_fetcher.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cctype>
#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>
#include <thread>

using json = nlohmann::json;

namespace devfinder {
namespace crawler {

namespace {
    constexpr int kPerPage = 30;
    constexpr int kMaxRateLimitRetries = 2;      // per topic, before the whole run is aborted
    constexpr int kMaxConsecutiveFailures = 3;   // e.g. network is down -> stop instead of hammering

    std::string trim(const std::string& s) {
        size_t b = 0, e = s.size();
        while (b < e && std::isspace(static_cast<unsigned char>(s[b]))) ++b;
        while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) --e;
        return s.substr(b, e - b);
    }
}

RepoFetcher::RepoFetcher(std::shared_ptr<IGithubClient> client) : client_(std::move(client)) {}

void RepoFetcher::pace() {
    if (!first_request_ && request_delay_ms_ > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(request_delay_ms_));
    }
    first_request_ = false;
}

std::vector<std::string> RepoFetcher::load_seed_topics(const std::string& path) {
    std::ifstream in(path);
    if (!in) {
        throw std::runtime_error("Cannot open seed topics file: " + path);
    }
    std::vector<std::string> topics;
    std::unordered_set<std::string> seen;
    std::string line;
    while (std::getline(in, line)) {
        auto hash = line.find('#');
        if (hash != std::string::npos) line.erase(hash);
        std::string topic = trim(line);
        std::transform(topic.begin(), topic.end(), topic.begin(),
                       [](unsigned char c) { return std::tolower(c); });
        if (topic.empty()) continue;
        if (seen.insert(topic).second) topics.push_back(topic);
    }
    return topics;
}

std::vector<common::RepoMetadata> RepoFetcher::fetch_by_topic(const std::string& topic, int max_results,
                                                              const std::unordered_set<std::string>& known_ids) {
    std::vector<common::RepoMetadata> results;
    int duplicates = 0;
    fetch_into(topic, max_results, known_ids, results, duplicates);
    return results;
}

void RepoFetcher::fetch_into(const std::string& topic, int max_results,
                             const std::unordered_set<std::string>& known_ids,
                             std::vector<common::RepoMetadata>& results, int& duplicates) {
    int page = 1;
    int examined = 0;   // repos looked at from search results (new + duplicates); bounds paging
    std::unordered_set<std::string> seen_in_topic;

    while (examined < max_results) {
        std::stringstream endpoint;
        std::string encoded_topic = topic;
        std::replace(encoded_topic.begin(), encoded_topic.end(), ' ', '+');
        endpoint << "/search/repositories?q=topic:" << encoded_topic << "&sort=stars&order=desc&per_page=" << kPerPage << "&page=" << page;

        pace();
        std::string response = client_->get(endpoint.str());
        if (response.empty()) break;

        json j;
        try {
            j = json::parse(response);
        } catch (const json::parse_error& e) {
            throw std::runtime_error("Malformed JSON response from GitHub: " + std::string(e.what()));
        }

        if (!j.contains("items") || !j["items"].is_array()) {
            break;
        }

        auto items = j["items"];
        if (items.empty()) break;

        for (const auto& item : items) {
            if (examined >= max_results) break;
            ++examined;

            common::RepoMetadata meta;
            meta.id = item.contains("full_name") && item["full_name"].is_string() ? item["full_name"].get<std::string>() : "";

            if (!meta.id.empty() && (known_ids.count(meta.id) || !seen_in_topic.insert(meta.id).second)) {
                ++duplicates;
                continue;
            }

            meta.name = item.contains("name") && item["name"].is_string() ? item["name"].get<std::string>() : "";
            if (item.contains("owner") && item["owner"].is_object()) {
                meta.owner = item["owner"].contains("login") && item["owner"]["login"].is_string() ? item["owner"]["login"].get<std::string>() : "";
            }
            meta.description = item.contains("description") && item["description"].is_string() ? item["description"].get<std::string>() : "";
            meta.primary_language = item.contains("language") && item["language"].is_string() ? item["language"].get<std::string>() : "";
            meta.stars = item.value("stargazers_count", 0);
            meta.forks = item.value("forks_count", 0);
            meta.open_issues = item.value("open_issues_count", 0);
            meta.last_commit_date = item.contains("updated_at") && item["updated_at"].is_string() ? item["updated_at"].get<std::string>() : "";

            if (item.contains("topics") && item["topics"].is_array()) {
                for (const auto& t : item["topics"]) {
                    if (t.is_string()) meta.topics.push_back(t.get<std::string>());
                }
            }
            if (item.contains("license") && item["license"].is_object()) {
                meta.license = item["license"].value("key", "");
            }

            if (enrich_devops_) {
                meta.has_dockerfile = check_has_dockerfile(meta.owner, meta.name);
                meta.has_ci_workflow = check_has_ci_workflow(meta.owner, meta.name);
            }

            results.push_back(meta);
        }

        // A short page means GitHub has no more results for this topic.
        if (static_cast<int>(items.size()) < kPerPage) break;
        page++;
    }
}

IngestSummary RepoFetcher::fetch_topics(
    const std::vector<std::string>& topics, int max_per_topic,
    std::unordered_set<std::string>& known_ids,
    const std::function<void(const std::vector<common::RepoMetadata>&)>& on_topic_batch) {

    IngestSummary summary;
    int consecutive_failures = 0;
    const size_t total = topics.size();

    for (size_t i = 0; i < total; ++i) {
        const std::string& topic = topics[i];
        ++summary.topics_attempted;
        std::cout << "[" << (i + 1) << "/" << total << "] topic: " << topic << " ... " << std::flush;

        std::vector<common::RepoMetadata> batch;
        int duplicates = 0;
        bool ok = false;
        int rate_limit_retries = 0;

        while (true) {
            try {
                fetch_into(topic, max_per_topic, known_ids, batch, duplicates);
                ok = true;
                break;
            } catch (const RateLimitError& e) {
                if (rate_limit_backoff_s_ > 0 && rate_limit_retries < kMaxRateLimitRetries) {
                    ++rate_limit_retries;
                    std::cout << "\n  rate limited (" << e.what() << "); waiting " << rate_limit_backoff_s_
                              << "s before retry " << rate_limit_retries << "/" << kMaxRateLimitRetries << " ... " << std::flush;
                    std::this_thread::sleep_for(std::chrono::seconds(rate_limit_backoff_s_));
                    // Keep what the failed attempt collected: persist it now and mark its ids as known,
                    // so the retry neither loses nor re-adds them.
                    if (!batch.empty()) {
                        if (on_topic_batch) on_topic_batch(batch);
                        for (const auto& r : batch) known_ids.insert(r.id);
                        summary.new_repos += static_cast<int>(batch.size());
                        batch.clear();
                    }
                    continue;
                }
                std::cout << "rate limit not clearing; aborting remaining topics.\n";
                summary.aborted = true;
                ++summary.topics_failed;
                break;
            } catch (const std::exception& e) {
                std::cout << "FAILED (" << e.what() << ")\n";
                ++summary.topics_failed;
                ++consecutive_failures;
                break;
            }
        }

        if (!ok && !batch.empty()) {
            // Partial results from a topic that failed midway are still worth keeping.
            if (on_topic_batch) on_topic_batch(batch);
            for (const auto& r : batch) known_ids.insert(r.id);
            summary.new_repos += static_cast<int>(batch.size());
        }
        if (summary.aborted) break;
        if (!ok) {
            if (consecutive_failures >= kMaxConsecutiveFailures) {
                std::cout << "Too many consecutive failures; aborting remaining topics.\n";
                summary.aborted = true;
                break;
            }
            continue;
        }
        consecutive_failures = 0;

        if (batch.empty() && duplicates == 0) {
            std::cout << "no repositories found, skipping\n";
            ++summary.topics_empty;
            continue;
        }

        // Persist this topic's repos right away so later failures cannot lose them.
        if (!batch.empty() && on_topic_batch) on_topic_batch(batch);
        for (const auto& r : batch) known_ids.insert(r.id);

        ++summary.topics_succeeded;
        summary.new_repos += static_cast<int>(batch.size());
        summary.duplicate_repos += duplicates;
        std::cout << batch.size() << " new, " << duplicates << " duplicate\n";
    }

    return summary;
}

bool RepoFetcher::check_has_dockerfile(const std::string& owner, const std::string& repo) {
    std::string endpoint = "/repos/" + owner + "/" + repo + "/contents/Dockerfile";
    try {
        std::string resp = client_->get(endpoint);
        return !resp.empty();
    } catch (const std::exception&) {
        return false;
    }
}

bool RepoFetcher::check_has_ci_workflow(const std::string& owner, const std::string& repo) {
    std::string endpoint = "/repos/" + owner + "/" + repo + "/contents/.github/workflows";
    try {
        std::string resp = client_->get(endpoint);
        return !resp.empty(); 
    } catch (const std::exception&) {
        return false;
    }
}

} // namespace crawler
} // namespace devfinder
