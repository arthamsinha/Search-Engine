#pragma once
#include "devfinder/common/models.hpp"
#include <string>

namespace devfinder {
namespace ranking {

struct ScoreWeights {
    double relevance = 0.4;
    double popularity = 0.2;
    double maintenance = 0.2;
    double devops = 0.2;
};

struct ScoreBreakdown {
    std::string repo_id;
    std::string owner;
    std::string name;
    double relevance_score;
    double popularity_score;
    double maintenance_score;
    double devops_score;
    double final_score;
    std::vector<std::string> matched_terms;
    std::vector<std::string> missing_terms;
};

class HealthScorer {
public:
    explicit HealthScorer(ScoreWeights weights = ScoreWeights{});
    ScoreBreakdown compute(const common::RepoMetadata& repo, double bm25_score) const;
private:
    ScoreWeights weights_;
    double normalize_popularity(int stars, int forks) const;
    double normalize_maintenance(const std::string& last_commit_date) const;
    double normalize_devops(const common::RepoMetadata& repo) const;
};

} // namespace ranking
} // namespace devfinder
