#include <gtest/gtest.h>
#include "devfinder/index/inverted_index.hpp"
#include "devfinder/index/index_storage.hpp"

using namespace devfinder::index;

TEST(InvertedIndexTest, PostingListsAndStats) {
    InvertedIndex idx;
    
    idx.add_document("doc1", {"hello", "world", "hello"});
    idx.add_document("doc2", {"world", "testing"});
    
    EXPECT_EQ(idx.document_count(), 2);
    EXPECT_EQ(idx.doc_length("doc1"), 3);
    EXPECT_EQ(idx.doc_length("doc2"), 2);
    EXPECT_DOUBLE_EQ(idx.average_doc_length(), 2.5);
    
    auto postings = idx.postings_for("hello");
    ASSERT_EQ(postings.size(), 1);
    EXPECT_EQ(postings[0].doc_id, "doc1");
    EXPECT_EQ(postings[0].term_frequency, 2);
    EXPECT_EQ(postings[0].positions.size(), 2);
    EXPECT_EQ(postings[0].positions[0], 0);
    EXPECT_EQ(postings[0].positions[1], 2);
    
    postings = idx.postings_for("world");
    ASSERT_EQ(postings.size(), 2);
    EXPECT_EQ(postings[0].doc_id, "doc1");
    EXPECT_EQ(postings[1].doc_id, "doc2");
}

TEST(InvertedIndexTest, UnknownTermEmptyResult) {
    InvertedIndex idx;
    idx.add_document("doc1", {"hello", "world"});
    auto postings = idx.postings_for("missing");
    EXPECT_TRUE(postings.empty());
}

TEST(InvertedIndexTest, SaveAndLoadRoundTrip) {
    InvertedIndex idx;
    idx.add_document("doc1", {"a", "b", "c", "a"});
    idx.add_document("doc2", {"c", "d"});
    
    std::string path = "test_index.bin";
    IndexStorage::save(idx, path);
    
    InvertedIndex loaded = IndexStorage::load(path);
    
    EXPECT_EQ(idx.document_count(), loaded.document_count());
    EXPECT_DOUBLE_EQ(idx.average_doc_length(), loaded.average_doc_length());
    EXPECT_EQ(idx.doc_length("doc1"), loaded.doc_length("doc1"));
    
    auto orig_a = idx.postings_for("a");
    auto loaded_a = loaded.postings_for("a");
    
    ASSERT_EQ(orig_a.size(), loaded_a.size());
    EXPECT_EQ(orig_a[0], loaded_a[0]);
    
    std::remove(path.c_str());
}
