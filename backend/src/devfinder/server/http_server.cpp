#include "devfinder/server/http_server.hpp"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace devfinder {
namespace server {

HttpServer::HttpServer(const query::QueryEngine& engine, const autocomplete::Trie& trie)
    : engine_(engine), trie_(trie) {
    setup_routes();
}

void HttpServer::setup_routes() {
    auto set_cors = [](httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
    };

    svr_.Get("/health", [&set_cors](const httplib::Request&, httplib::Response& res) {
        set_cors(res);
        json response = {{"status", "ok"}};
        res.set_content(response.dump(), "application/json");
    });

    svr_.Get("/api/search", [this, &set_cors](const httplib::Request& req, httplib::Response& res) {
        set_cors(res);
        if (!req.has_param("q")) {
            json error = {{"error", "Missing required parameter 'q'"}};
            res.status = 400;
            res.set_content(error.dump(), "application/json");
            return;
        }

        std::string q = req.get_param_value("q");
        try {
            auto results = engine_.search(q);

            // ── Relevance (BM25) min-max normalization across this result set ──
            double min_rel = results.empty() ? 0.0 : results[0].relevance_score;
            double max_rel = min_rel;
            for (const auto& r : results) {
                if (r.relevance_score < min_rel) min_rel = r.relevance_score;
                if (r.relevance_score > max_rel) max_rel = r.relevance_score;
            }
            double rel_range = max_rel - min_rel;

            auto clamp01 = [](double v) { return v < 0.0 ? 0.0 : (v > 1.0 ? 1.0 : v); };

            json json_results = json::array();
            for (const auto& breakdown : results) {
                // Relevance: per-result-set min-max; fall back to fixed ceiling for 1 or 2 results
                double norm_relevance = 0.0;
                if (results.size() < 3) {
                    // Normalize against a fixed ceiling instead of a per-result-set range for 1-2 results.
                    // 15.0 is an approximate 95th-percentile BM25 ceiling observed during manual query testing.
                    const double FIXED_CEILING = 15.0;
                    norm_relevance = clamp01(breakdown.relevance_score / FIXED_CEILING);
                } else {
                    norm_relevance = (rel_range < 1e-9)
                        ? 0.5
                        : (breakdown.relevance_score - min_rel) / rel_range;
                }

                // Other scores: already bounded [0,1] by HealthScorer — just clamp defensively
                double norm_popularity   = clamp01(breakdown.popularity_score);
                double norm_maintenance  = clamp01(breakdown.maintenance_score);
                double norm_devops       = clamp01(breakdown.devops_score);

                // Determine which normalised sub-score drove the result
                std::string driven_by = "relevance";
                double top = norm_relevance;
                if (norm_popularity  > top) { driven_by = "popularity";   top = norm_popularity; }
                if (norm_maintenance > top) { driven_by = "maintenance";  top = norm_maintenance; }
                if (norm_devops      > top) { driven_by = "DevOps";                               }

                json_results.push_back({
                    {"repo_id",   breakdown.repo_id},
                    {"owner",     breakdown.owner},
                    {"name",      breakdown.name},
                    {"final_score", breakdown.final_score},
                    {"driven_by", driven_by},
                    {"matched_terms", breakdown.matched_terms},
                    {"missing_terms", breakdown.missing_terms},
                    {"breakdown", {
                        {"relevance",   norm_relevance},
                        {"popularity",  norm_popularity},
                        {"maintenance", norm_maintenance},
                        {"devops",      norm_devops}
                    }}
                });
            }

            json response = {
                {"query", q},
                {"results", json_results}
            };
            res.set_content(response.dump(), "application/json");
        } catch (const std::exception& e) {
            json error = {{"error", "Internal Server Error"}};
            res.status = 500;
            res.set_content(error.dump(), "application/json");
        }
    });


    svr_.Get("/api/autocomplete", [this, &set_cors](const httplib::Request& req, httplib::Response& res) {
        set_cors(res);
        if (!req.has_param("prefix")) {
            json error = {{"error", "Missing required parameter 'prefix'"}};
            res.status = 400;
            res.set_content(error.dump(), "application/json");
            return;
        }
        
        std::string prefix = req.get_param_value("prefix");
        try {
            auto suggestions = trie_.suggest(prefix);
            json response = {
                {"prefix", prefix},
                {"suggestions", suggestions}
            };
            res.set_content(response.dump(), "application/json");
        } catch (const std::exception& e) {
            json error = {{"error", "Internal Server Error"}};
            res.status = 500;
            res.set_content(error.dump(), "application/json");
        }
    });
}

void HttpServer::listen(const std::string& host, int port) {
    svr_.listen(host.c_str(), port);
}

httplib::Server& HttpServer::get_raw_server() {
    return svr_;
}

} // namespace server
} // namespace devfinder
