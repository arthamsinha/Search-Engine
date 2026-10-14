#include "devfinder/crawler/github_client.hpp"
#include <curl/curl.h>

namespace devfinder {
namespace crawler {

namespace {
    size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
        ((std::string*)userp)->append((char*)contents, size * nmemb);
        return size * nmemb;
    }
}

GithubClient::GithubClient(std::string token) : token_(std::move(token)) {}

std::string GithubClient::get(const std::string& endpoint) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        throw std::runtime_error("Failed to initialize CURL");
    }

    std::string url = "https://api.github.com" + endpoint;
    std::string readBuffer;
    long http_code = 0;

    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, "User-Agent: DevFinder-Crawler/1.0");
    headers = curl_slist_append(headers, "Accept: application/vnd.github.v3+json");
    if (!token_.empty()) {
        std::string authHeader = "Authorization: Bearer " + token_;
        headers = curl_slist_append(headers, authHeader.c_str());
    }

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        throw std::runtime_error(std::string("CURL request failed: ") + curl_easy_strerror(res));
    }

    if (http_code == 403 || http_code == 429) {
        throw RateLimitError("GitHub API rate limit exceeded (HTTP " + std::to_string(http_code) + ")");
    }
    
    if (http_code < 200 || http_code >= 300) {
        if (http_code == 404) {
            return ""; // Not found, handle gracefully for file checks
        }
        throw std::runtime_error("GitHub API returned HTTP " + std::to_string(http_code) + "\n" + readBuffer);
    }

    return readBuffer;
}

} // namespace crawler
} // namespace devfinder
