#include <gtest/gtest.h>
#include "devfinder/autocomplete/trie.hpp"

using namespace devfinder::autocomplete;

TEST(TrieTest, BasicInsertionAndSuggestion) {
    Trie trie;
    trie.insert("kubernetes", 10);
    trie.insert("kubectl", 5);
    trie.insert("kubeflow", 8);
    
    auto results = trie.suggest("kube");
    ASSERT_EQ(results.size(), 3);
    EXPECT_EQ(results[0], "kubernetes");
    EXPECT_EQ(results[1], "kubeflow");
    EXPECT_EQ(results[2], "kubectl");
}

TEST(TrieTest, CaseInsensitive) {
    Trie trie;
    trie.insert("KuberNetes", 10);
    
    auto results1 = trie.suggest("kube");
    ASSERT_EQ(results1.size(), 1);
    EXPECT_EQ(results1[0], "kubernetes");
    
    auto results2 = trie.suggest("KUBE");
    ASSERT_EQ(results2.size(), 1);
    EXPECT_EQ(results2[0], "kubernetes");
}

TEST(TrieTest, NoMatchEmpty) {
    Trie trie;
    trie.insert("docker", 5);
    
    auto results = trie.suggest("kubernetes");
    EXPECT_TRUE(results.empty());
}
