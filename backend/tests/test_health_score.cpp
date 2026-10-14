#include <gtest/gtest.h>
#include "devfinder/ranking/health_score.hpp"
#include <chrono>
#include <iomanip>
#include <sstream>

using namespace devfinder::ranking;
using namespace devfinder::common;

std::string get_date_days_ago(int days) {
    auto t = std::chrono::system_clock::now() - std::chrono::hours(24 * days);
    auto time_t = std::chrono::system_clock::to_time_t(t);
    std::tm tm_val = *std::localtime(&time_t);
    std::stringstream ss;
    ss << std::put_time(&tm_val, "%Y-%m-%dT%H:%M:%SZ");
    return ss.str();
}

TEST(HealthScoreTest, StaleVsActiveMaintenance) {
    HealthScorer scorer;
    
    RepoMetadata stale_repo;
    stale_repo.stars = 10000;
    stale_repo.last_commit_date = get_date_days_ago(1000); 
    
    RepoMetadata active_repo;
    active_repo.stars = 10;
    active_repo.last_commit_date = get_date_days_ago(2); 
    
    auto stale_breakdown = scorer.compute(stale_repo, 1.0);
    auto active_breakdown = scorer.compute(active_repo, 1.0);
    
    EXPECT_LT(stale_breakdown.maintenance_score, active_breakdown.maintenance_score);
}

TEST(HealthScoreTest, DevOpsScoreScaling) {
    HealthScorer scorer;
    
    RepoMetadata no_flags;
    RepoMetadata all_flags;
    all_flags.has_dockerfile = true;
    all_flags.has_ci_workflow = true;
    all_flags.has_k8s_manifests = true;
    all_flags.has_tests_dir = true;
    
    auto no_flags_breakdown = scorer.compute(no_flags, 1.0);
    auto all_flags_breakdown = scorer.compute(all_flags, 1.0);
    
    EXPECT_DOUBLE_EQ(no_flags_breakdown.devops_score, 0.0);
    EXPECT_DOUBLE_EQ(all_flags_breakdown.devops_score, 1.0);
    EXPECT_GT(all_flags_breakdown.devops_score, no_flags_breakdown.devops_score);
}

TEST(HealthScoreTest, FinalScoreMatchesManual) {
    HealthScorer scorer; 
    
    RepoMetadata repo;
    repo.stars = 99; // log10(100) = 2.0 -> / 5.0 = 0.4
    repo.forks = 0;
    repo.has_dockerfile = true;
    repo.has_ci_workflow = true; // devops = 0.5
    repo.last_commit_date = get_date_days_ago(0); 
    
    double bm25 = 1.5; 
    
    auto bd = scorer.compute(repo, bm25);
    
    EXPECT_DOUBLE_EQ(bd.relevance_score, 1.5);
    EXPECT_NEAR(bd.popularity_score, 0.4, 1e-5);
    EXPECT_NEAR(bd.maintenance_score, 1.0, 1e-1); 
    EXPECT_DOUBLE_EQ(bd.devops_score, 0.5);
    
    double expected_final = (1.5 * 0.4) + (0.4 * 0.2) + (1.0 * 0.2) + (0.5 * 0.2);
    EXPECT_NEAR(bd.final_score, expected_final, 1e-1);
}

TEST(HealthScoreTest, WeightChangesPredictably) {
    ScoreWeights pop_heavy{0.0, 1.0, 0.0, 0.0};
    HealthScorer pop_scorer(pop_heavy);
    
    RepoMetadata high_star; high_star.stars = 10000;
    RepoMetadata low_star;  low_star.stars = 10;
    
    auto high_pop = pop_scorer.compute(high_star, 0.0).final_score;
    auto low_pop = pop_scorer.compute(low_star, 0.0).final_score;
    
    EXPECT_GT(high_pop, low_pop);
    
    ScoreWeights rel_heavy{1.0, 0.0, 0.0, 0.0};
    HealthScorer rel_scorer(rel_heavy);
    
    auto high_rel = rel_scorer.compute(high_star, 2.0).final_score; 
    auto low_rel = rel_scorer.compute(low_star, 1.0).final_score;
    
    EXPECT_GT(high_rel, low_rel);
}
