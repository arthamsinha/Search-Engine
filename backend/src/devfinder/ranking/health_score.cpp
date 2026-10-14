#include "devfinder/ranking/health_score.hpp"
#include <cmath>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <chrono>

namespace devfinder {
namespace ranking {

HealthScorer::HealthScorer(ScoreWeights weights) : weights_(weights) {}

double HealthScorer::normalize_popularity(int stars, int forks) const {
    double raw = std::log10(stars + forks + 1.0);
    return std::min(raw / 5.0, 1.0); // maxes out at ~100k
}

double HealthScorer::normalize_maintenance(const std::string& last_commit_date) const {
    if (last_commit_date.empty()) return 0.0;

    std::tm t = {};
    std::istringstream ss(last_commit_date);
    ss >> std::get_time(&t, "%Y-%m-%dT%H:%M:%S");
    
    if (ss.fail()) return 0.0;
    
    auto commit_time = std::chrono::system_clock::from_time_t(std::mktime(&t));
    auto now = std::chrono::system_clock::now();
    
    double days = 0.0;
    if (now > commit_time) {
        auto duration = std::chrono::duration_cast<std::chrono::hours>(now - commit_time);
        days = duration.count() / 24.0;
    }
    
    // Exponential decay: e^(-lambda * days). Half-life of 1 year (~365 days) -> lambda = ln(2)/365
    double lambda = 0.693147 / 365.0;
    return std::exp(-lambda * days);
}

double HealthScorer::normalize_devops(const common::RepoMetadata& repo) const {
    double score = 0.0;
    if (repo.has_dockerfile) score += 0.25;
    if (repo.has_ci_workflow) score += 0.25;
    if (repo.has_k8s_manifests) score += 0.25;
    if (repo.has_tests_dir) score += 0.25;
    return score;
}

ScoreBreakdown HealthScorer::compute(const common::RepoMetadata& repo, double bm25_score) const {
    ScoreBreakdown breakdown;
    breakdown.owner = repo.owner;
    breakdown.name = repo.name;
    breakdown.relevance_score = bm25_score;
    breakdown.popularity_score = normalize_popularity(repo.stars, repo.forks);
    breakdown.maintenance_score = normalize_maintenance(repo.last_commit_date);
    breakdown.devops_score = normalize_devops(repo);
    
    breakdown.final_score = 
        (breakdown.relevance_score * weights_.relevance) +
        (breakdown.popularity_score * weights_.popularity) +
        (breakdown.maintenance_score * weights_.maintenance) +
        (breakdown.devops_score * weights_.devops);
        
    return breakdown;
}

} // namespace ranking
} // namespace devfinder
