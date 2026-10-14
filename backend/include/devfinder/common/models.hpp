#pragma once
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace devfinder {
namespace common {

struct RepoMetadata {
    std::string id;
    std::string name;
    std::string owner;
    std::string description;
    std::string readme_text;
    std::string primary_language;
    std::vector<std::string> topics;
    int stars = 0;
    int forks = 0;
    int open_issues = 0;
    int contributors_count = 0;
    std::string last_commit_date;
    std::string license;
    bool has_dockerfile = false;
    bool has_ci_workflow = false;
    bool has_k8s_manifests = false;
    bool has_tests_dir = false;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(
    RepoMetadata, id, name, owner, description, readme_text, primary_language,
    topics, stars, forks, open_issues, contributors_count, last_commit_date,
    license, has_dockerfile, has_ci_workflow, has_k8s_manifests, has_tests_dir
)

} // namespace common
} // namespace devfinder
