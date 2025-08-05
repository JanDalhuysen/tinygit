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


// Simple SHA-1 implementation for Git
std::string sha1(const std::string& data) {
    // Initial hash values (from FIPS 180-1)
    uint32_t h0 = 0x67452301;
    uint32_t h1 = 0xEFCDAB89;
    uint32_t h2 = 0x98BADCFE;
    uint32_t h3 = 0x10325476;
    uint32_t h4 = 0xC3D2E1F0;

    // Pre-processing: padding
    std::vector<uint8_t> padded = std::vector<uint8_t>(data.begin(), data.end());
    size_t orig_len = padded.size();
    
    // Append bit '1'
    padded.push_back(0x80);
    
    // Append zeros
    while ((padded.size() * 8) % 512 != 448) {
        padded.push_back(0x00);
    }
    
    // Append original length in bits as 64-bit big-endian integer
    uint64_t bit_len = orig_len * 8;
    for (int i = 7; i >= 0; i--) {
        padded.push_back((bit_len >> (i * 8)) & 0xFF);
    }

    // Process blocks of 512 bits (64 bytes)
    for (size_t offset = 0; offset < padded.size(); offset += 64) {
        // Copy block into 16 32-bit words
        uint32_t w[80];
        for (int i = 0; i < 16; i++) {
            w[i] = 0;
            for (int j = 0; j < 4; j++) {
                w[i] |= ((uint32_t)padded[offset + i*4 + j]) << (24 - j*8);
            }
        }
        
        // Extend to 80 words
        for (int i = 16; i < 80; i++) {
            w[i] = (w[i-3] ^ w[i-8] ^ w[i-14] ^ w[i-16]);
            w[i] = (w[i] << 1) | (w[i] >> 31);
        }
        
        // Main loop
        uint32_t a = h0, b = h1, c = h2, d = h3, e = h4;
        
        for (int i = 0; i < 80; i++) {
            uint32_t f, k;
            if (i < 20) {
                f = (b & c) | ((~b) & d);
                k = 0x5A827999;
            } else if (i < 40) {
                f = b ^ c ^ d;
                k = 0x6ED9EBA1;
            } else if (i < 60) {
                f = (b & c) | (b & d) | (c & d);
                k = 0x8F1BBCDC;
            } else {
                f = b ^ c ^ d;
                k = 0xCA62C1D6;
            }
            
            uint32_t temp = ((a << 5) | (a >> 27)) + f + e + k + w[i];
            e = d;
            d = c;
            c = (b << 30) | (b >> 2);
            b = a;
            a = temp;
        }
        
        // Add to hash
        h0 += a;
        h1 += b;
        h2 += c;
        h3 += d;
        h4 += e;
    }
    
    // Convert to hex string
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    oss << std::setw(8) << h0;
    oss << std::setw(8) << h1;
    oss << std::setw(8) << h2;
    oss << std::setw(8) << h3;
    oss << std::setw(8) << h4;
    
    return oss.str();
}



std::string write_object(const std::string& data, const std::string& type) {
    // Format: "<type> <size>\0<data>"
    std::string header = type + " " + std::to_string(data.size()) + "\0";
    std::string store = header + data;
    
    // Compute SHA1 of the entire store content
    std::string sha = sha1(store);
    
    // Create directory and write compressed object
    std::string dir = ".tinygit/objects/" + sha.substr(0, 2);
    std::string file = dir + "/" + sha.substr(2);
    fs::create_directories(dir);
    
    // Compress with zlib
    z_stream zs = {};
    deflateInit(&zs, Z_DEFAULT_COMPRESSION);
    zs.next_in = (Bytef*)store.data();
    zs.avail_in = store.size();
    
    std::vector<Bytef> buffer(1024);
    std::vector<Bytef> compressed;
    
    int ret;
    do {
        zs.next_out = buffer.data();
        zs.avail_out = buffer.size();
        ret = deflate(&zs, Z_FINISH);
        compressed.insert(compressed.end(), buffer.begin(), buffer.end() - zs.avail_out);
    } while (ret == Z_OK);
    
    deflateEnd(&zs);
    
    // Write compressed data
    std::ofstream out(file, std::ios::binary);
    out.write((char*)compressed.data(), compressed.size());
    
    return sha;
}



std::string read_object(const std::string& sha) {
    // Read and decompress object
    std::string file = ".tinygit/objects/" + sha.substr(0, 2) + "/" + sha.substr(2);
    std::ifstream in(file, std::ios::binary);
    std::vector<char> compressed((std::istreambuf_iterator<char>(in)),
                                 std::istreambuf_iterator<char>());
    
    z_stream zs = {};
    inflateInit(&zs);
    zs.next_in = (Bytef*)compressed.data();
    zs.avail_in = compressed.size();
    
    std::vector<Bytef> buffer(1024);
    std::string result;
    
    int ret;
    do {
        zs.next_out = buffer.data();
        zs.avail_out = buffer.size();
        ret = inflate(&zs, Z_NO_FLUSH);
        result.append((char*)buffer.data(), buffer.size() - zs.avail_out);
    } while (ret == Z_OK);
    
    inflateEnd(&zs);
    
    // Skip header to get to data
    size_t null_pos = result.find('\0');
    return result.substr(null_pos + 1);
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
