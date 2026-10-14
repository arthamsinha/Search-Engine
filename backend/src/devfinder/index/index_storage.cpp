#include "devfinder/index/index_storage.hpp"
#include <fstream>
#include <stdexcept>
#include <cstdint>

namespace devfinder {
namespace index {

namespace {

void write_int(std::ofstream& out, int32_t val) {
    out.write(reinterpret_cast<const char*>(&val), sizeof(val));
}

int32_t read_int(std::ifstream& in) {
    int32_t val;
    in.read(reinterpret_cast<char*>(&val), sizeof(val));
    return val;
}

void write_string(std::ofstream& out, const std::string& str) {
    write_int(out, static_cast<int32_t>(str.size()));
    if (!str.empty()) {
        out.write(str.data(), str.size());
    }
}

std::string read_string(std::ifstream& in) {
    int32_t len = read_int(in);
    if (len < 0) throw std::runtime_error("Invalid string length");
    std::string str(len, '\0');
    if (len > 0) {
        in.read(&str[0], len);
    }
    return str;
}

} // namespace

void IndexStorage::save(const InvertedIndex& index, const std::string& path) {
    std::ofstream out(path, std::ios::binary);
    if (!out) throw std::runtime_error("Could not open file for writing: " + path);

    const auto& doc_lengths = index.get_raw_doc_lengths();
    write_int(out, doc_lengths.size());
    for (const auto& [doc_id, length] : doc_lengths) {
        write_string(out, doc_id);
        write_int(out, length);
    }

    const auto& index_data = index.get_raw_index();
    write_int(out, index_data.size());
    for (const auto& [term, postings] : index_data) {
        write_string(out, term);
        write_int(out, postings.size());
        for (const auto& p : postings) {
            write_string(out, p.doc_id);
            write_int(out, p.term_frequency);
            write_int(out, p.positions.size());
            for (int pos : p.positions) {
                write_int(out, pos);
            }
        }
    }
}

InvertedIndex IndexStorage::load(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("Could not open file for reading: " + path);

    std::unordered_map<std::string, int> doc_lengths;
    int32_t doc_count = read_int(in);
    for (int i = 0; i < doc_count; ++i) {
        std::string doc_id = read_string(in);
        int32_t length = read_int(in);
        doc_lengths[doc_id] = length;
    }

    std::unordered_map<std::string, std::vector<Posting>> index_data;
    int32_t term_count = read_int(in);
    for (int i = 0; i < term_count; ++i) {
        std::string term = read_string(in);
        int32_t postings_count = read_int(in);
        
        std::vector<Posting> postings;
        postings.reserve(postings_count);
        for (int j = 0; j < postings_count; ++j) {
            Posting p;
            p.doc_id = read_string(in);
            p.term_frequency = read_int(in);
            
            int32_t positions_count = read_int(in);
            p.positions.reserve(positions_count);
            for (int k = 0; k < positions_count; ++k) {
                p.positions.push_back(read_int(in));
            }
            postings.push_back(p);
        }
        index_data[term] = std::move(postings);
    }

    InvertedIndex idx;
    idx.set_raw_data(std::move(index_data), std::move(doc_lengths));
    return idx;
}

} // namespace index
} // namespace devfinder
