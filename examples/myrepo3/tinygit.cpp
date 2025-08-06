// tinygit.cpp
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include <iomanip>
#include <ctime>
#include <zlib.h>
#include <filesystem>

namespace fs = std::filesystem;

std::string sha1(const std::string& data);            // see below
std::string write_object(const std::string& data, const std::string& type);
std::string read_object(const std::string& sha);      // reverse lookup





#include <openssl/sha.h>   // link with -lcrypto

std::string sha1(const std::string& data) {
    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1((const unsigned char*)data.c_str(), data.size(), hash);
    std::ostringstream oss;
    for (int i = 0; i < SHA_DIGEST_LENGTH; ++i)
        oss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    return oss.str();
}

std::string write_object(const std::string& data, const std::string& type) {
    std::string store = type + " " + std::to_string(data.size()) + '\0' + data;
    std::string sha = sha1(store);

    uLong bound = compressBound(store.size());
    std::vector<Bytef> compressed(bound);
    uLong len = bound;
    compress(compressed.data(), &len, (const Bytef*)store.data(), store.size());

    fs::path file(".tinygit/objects/" + sha.substr(0,2) + "/" + sha.substr(2));
    fs::create_directories(file.parent_path());
    std::ofstream(file, std::ios::binary)
        .write((char*)compressed.data(), len);
    
    return sha;
}

std::string read_object(const std::string& sha) {
    fs::path file(".tinygit/objects/" + sha.substr(0,2) + "/" + sha.substr(2));
    std::ifstream in(file, std::ios::binary);
    if (!in) return "";
    
    std::vector<char> compressed((std::istreambuf_iterator<char>(in)),
                                 std::istreambuf_iterator<char>());
    
    // Decompress
    uLong len = compressed.size();
    uLong decompressed_size = 1024 * 1024; // Initial guess
    std::vector<Bytef> decompressed(decompressed_size);
    
    int ret;
    while ((ret = uncompress(decompressed.data(), &decompressed_size, 
                             (const Bytef*)compressed.data(), len)) == Z_BUF_ERROR) {
        decompressed_size *= 2;
        decompressed.resize(decompressed_size);
    }
    
    if (ret != Z_OK) return "";
    
    std::string store((char*)decompressed.data(), decompressed_size);
    
    // Extract data (skip header)
    size_t null_pos = store.find('\0');
    if (null_pos == std::string::npos) return "";
    
    return store.substr(null_pos + 1);
}

int main(int argc, char* argv[]) {
    if (argc < 2) { std::cerr << "usage: tinygit <command>\n"; return 1; }
    std::string cmd = argv[1];

    if (cmd == "init") {
        fs::create_directories(".tinygit/objects");
        fs::create_directories(".tinygit/refs/heads");
        std::ofstream(".tinygit/HEAD") << "ref: refs/heads/master\n";
        std::cout << "Initialized empty tinygit repo\n";
    }
    else if (cmd == "add" && argc == 3) {
        std::ifstream in(argv[2], std::ios::binary);
        std::string content((std::istreambuf_iterator<char>(in)),
                             std::istreambuf_iterator<char>());
        write_object(content, "blob");
    }
    else if (cmd == "commit" && argc == 4 && std::string(argv[2]) == "-m") {
        // 1. create tree (for simplicity: single root dir with staged blobs)
        std::string tree = "tree <sha>\n";            // TODO build real tree
        // 2. create commit object
        std::ostringstream commit;
        commit << "tree " << "dummy_sha\n"
               << "author me <me@example.com> " << std::time(nullptr) << " +0000\n"
               << "committer me <me@example.com> " << std::time(nullptr) << " +0000\n\n"
               << argv[3] << "\n";
        std::string sha = write_object(commit.str(), "commit");

        // 3. update branch ref
        std::ofstream(".tinygit/refs/heads/master") << sha << "\n";
        std::cout << "[master " << sha.substr(0,7) << "] " << argv[3] << "\n";
    }
    else if (cmd == "log") {
        std::ifstream head(".tinygit/refs/heads/master");
        std::string sha; head >> sha;
        std::cout << read_object(sha) << "\n";
    }
    else {
        std::cerr << "unknown command\n";
    }
}
