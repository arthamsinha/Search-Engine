#include <gtest/gtest.h>
#include "devfinder/common/config.hpp"
#include "devfinder/common/models.hpp"
#include <fstream>
#include <nlohmann/json.hpp>

using namespace devfinder::common;

TEST(Sanity, TrueIsTrue) {
    EXPECT_TRUE(true);
}

TEST(ConfigTest, LoadValidEnv) {
    // Create a temporary .env file
    std::string env_path = "test_valid.env";
    {
        std::ofstream out(env_path);
        out << "GITHUB_TOKEN=secret_token\n";
        out << "SERVER_PORT=9090\n";
        out << "# A comment\n";
        out << "DATA_DIR=custom/data\n";
    }

    Config config = Config::load_from_env(env_path);
    EXPECT_EQ(config.github_token(), "secret_token");
    EXPECT_EQ(config.server_port(), 9090);
    EXPECT_EQ(config.data_dir(), "custom/data");

    std::remove(env_path.c_str());
}

TEST(ConfigTest, MissingEnvThrows) {
    EXPECT_THROW(Config::load_from_env("non_existent.env"), std::runtime_error);
}

TEST(ModelsTest, RepoMetadataJsonRoundTrip) {
    RepoMetadata m1;
    m1.id = "user/repo";
    m1.name = "repo";
    m1.owner = "user";
    m1.stars = 42;
    m1.topics = {"cpp", "search"};
    m1.has_dockerfile = true;

    nlohmann::json j = m1;
    RepoMetadata m2 = j.get<RepoMetadata>();

    EXPECT_EQ(m1.id, m2.id);
    EXPECT_EQ(m1.name, m2.name);
    EXPECT_EQ(m1.stars, m2.stars);
    EXPECT_EQ(m1.topics, m2.topics);
    EXPECT_EQ(m1.has_dockerfile, m2.has_dockerfile);
}
