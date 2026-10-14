#include <gtest/gtest.h>
#include "devfinder/preprocessing/tokenizer.hpp"

using namespace devfinder::preprocessing;

TEST(TokenizerTest, SplitKebabCase) {
    Tokenizer t;
    auto tokens = t.tokenize("react-native");
    ASSERT_EQ(tokens.size(), 2);
    EXPECT_EQ(tokens[0], "react");
    EXPECT_EQ(tokens[1], "native");
}

TEST(TokenizerTest, SplitCamelCase) {
    Tokenizer t;
    auto tokens = t.tokenize("getUserData");
    ASSERT_EQ(tokens.size(), 3);
    EXPECT_EQ(tokens[0], "get");
    EXPECT_EQ(tokens[1], "user");
    EXPECT_EQ(tokens[2], "data");
}

TEST(TokenizerTest, RemoveStopwords) {
    Tokenizer t;
    auto tokens = t.tokenize("the react and native of");
    ASSERT_EQ(tokens.size(), 2);
    EXPECT_EQ(tokens[0], "react");
    EXPECT_EQ(tokens[1], "native");
}

TEST(TokenizerTest, EmptyString) {
    Tokenizer t;
    auto tokens = t.tokenize("");
    EXPECT_TRUE(tokens.empty());
}

TEST(TokenizerTest, MixedPunctuation) {
    Tokenizer t;
    auto tokens = t.tokenize("hello, world! This is a test.");
    // "this", "is", "a" are stopwords.
    ASSERT_EQ(tokens.size(), 3);
    EXPECT_EQ(tokens[0], "hello");
    EXPECT_EQ(tokens[1], "world");
    EXPECT_EQ(tokens[2], "test");
}
