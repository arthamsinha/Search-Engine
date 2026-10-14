#pragma once
#include "devfinder/crawler/github_client.hpp"
#include "devfinder/common/models.hpp"
#include <vector>
#include <memory>
#include <string>
#include <functional>
#include <unordered_set>

namespace devfinder {
namespace crawler {

// Outcome of a multi-topic ingestion run (see RepoFetcher::fetch_topics).
struct IngestSummary {
    int topics_attempted = 0;
    int topics_succeeded = 0;     // returned >= 1 repo
    int topics_empty = 0;         // returned zero repos (skipped cleanly)
    int topics_failed = 0;        // network / malformed response errors
    int new_repos = 0;            // repos saved this run
    int duplicate_repos = 0;      // repos skipped: already fetched (this run or earlier)
    bool aborted = false;         // run stopped early (rate limit exhausted / repeated failures)
};

class RepoFetcher {
public:
    explicit RepoFetcher(std::shared_ptr<IGithubClient> client);

    // `known_ids` lists repos that were already fetched; they are not re-enriched
    // (saves API calls) and are not returned again.
    std::vector<common::RepoMetadata> fetch_by_topic(const std::string& topic, int max_results,
                                                     const std::unordered_set<std::string>& known_ids = {});

    // Reads a seed topic list: one topic per line, '#' starts a comment (used for group headings).
    // Topics are lower-cased, trimmed and de-duplicated, order preserved.
    static std::vector<std::string> load_seed_topics(const std::string& path);

    // Fetches every topic in `topics`, de-duplicating by repo id across topics (and against
    // `known_ids`). After each topic completes, `on_topic_batch` is called with its new repos so the
    // caller can persist them immediately -- a later failure never discards earlier topics' data.
    IngestSummary fetch_topics(const std::vector<std::string>& topics, int max_per_topic,
                               std::unordered_set<std::string>& known_ids,
                               const std::function<void(const std::vector<common::RepoMetadata>&)>& on_topic_batch);

    // Pause between Search API requests (0 = none). GitHub allows 10 search req/min unauthenticated, 30 with a token.
    void set_request_delay_ms(int ms) { request_delay_ms_ = ms; }
    // Seconds to wait before retrying a topic after a rate limit error (0 = do not wait / retry).
    void set_rate_limit_backoff_seconds(int s) { rate_limit_backoff_s_ = s; }
    // When false, Dockerfile / CI-workflow detection is skipped (2 extra API calls per repo, which
    // exhaust the 60 req/hour unauthenticated quota almost immediately).
    void set_enrich_devops(bool enabled) { enrich_devops_ = enabled; }

    bool check_has_dockerfile(const std::string& owner, const std::string& repo);
    bool check_has_ci_workflow(const std::string& owner, const std::string& repo);
private:
    void pace();
    // Appends to `results` as it goes, so partial results survive an exception thrown mid-way.
    void fetch_into(const std::string& topic, int max_results,
                    const std::unordered_set<std::string>& known_ids,
                    std::vector<common::RepoMetadata>& results, int& duplicates);

    std::shared_ptr<IGithubClient> client_;
    int request_delay_ms_ = 0;
    int rate_limit_backoff_s_ = 0;
    bool enrich_devops_ = true;
    bool first_request_ = true;
};

} // namespace crawler
} // namespace devfinder
