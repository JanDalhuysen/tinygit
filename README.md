```bash
g++ tinygit.cpp -lcrypto -lz -std=c++17 -o tinygit
```


```bash
sudo apt update
sudo apt install pkg-config
sudo apt install build-essential cmake zip unzip
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg/
./bootstrap-vcpkg.sh
./bootstrap-vcpkg.sh
./vcpkg integrate install
./vcpkg install zlib
./vcpkg install openssl
cd ..
cd myrepo/
vim CMakeLists.txt
cmake . -DCMAKE_TOOLCHAIN_FILE=/home/jan/vcpkg/scripts/buildsystems/vcpkg.cmake
make
```

The primary difference between the two files lies in the choice of implementation for two key operations: **SHA-1 hashing** and **zlib data compression**.

### Explanation of Differences

The first program (`tinygit1.cpp`) uses a manual, from-scratch implementation for these features, while the second program (`tinygit2.cpp`) relies on standard, external libraries, which is the more common and robust approach.


| Feature | `tinygit1.cpp` (Original Program 1) | `tinygit2.cpp` (Original Program 2) |
| :--- | :--- | :--- |
| **SHA-1 Hashing**| Implements the SHA-1 algorithm manually. This involves explicit steps for padding the input data, processing it in 512-bit blocks, and applying the specific logical functions and constants defined in the SHA-1 standard. | Uses the `SHA1` function from the **OpenSSL** library (`-lcrypto`). This delegates the complex and security-critical task of hash computation to a well-tested, standard library. |
| **Data Compression**| Uses the low-level, streaming API of the **zlib** library (`deflateInit`, `deflate`, `inflateInit`, `inflate`). This approach is more complex but memory-efficient, as it can process data in chunks without loading the entire content into memory at once. | Uses the high-level, one-shot buffer functions from **zlib** (`compress`, `uncompress`). This method is simpler to write and reads the entire data into a memory buffer before performing the compression or decompression in a single function call. |
| **Overall Approach** | Educational and self-contained. It shows how the underlying algorithms work. | Pragmatic and robust. It uses industry-standard libraries for core functionality, which is generally safer and more maintainable. |

The combined program below adopts the more robust and concise implementations from `tinygit2.cpp`, using the OpenSSL library for SHA-1 hashing and the simpler `compress`/`uncompress` functions from zlib.

-----

### Combined Program

`combined_tinygit.cpp` merges the functionality of both files, choosing the library-based approach for stability and conciseness. To compile it, you will need to link the OpenSSL crypto library and the zlib library.

**Example compilation command:**
`g++ combined_tinygit.cpp -o tinygit -lstdc++fs -lcrypto -lz`

```cpp
// combined_tinygit.cpp
//
// This program combines two versions of a tinygit implementation.
// It uses the OpenSSL library for SHA-1 hashing and the zlib library
// for data compression, following the more robust and common approach.
//
// To compile:
// g++ combined_tinygit.cpp -o tinygit -lstdc++fs -lcrypto -lz

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <iomanip>
#include <ctime>
#include <filesystem>

// Zlib for compression/decompression
#include <zlib.h>

// OpenSSL for SHA-1 hashing
#include <openssl/sha.h>

namespace fs = std::filesystem;

/**
 * @brief Computes the SHA-1 hash of a string using the OpenSSL library.
 * @param data The input string data.
 * @return A 40-character hexadecimal string representing the SHA-1 hash.
 */
std::string sha1(const std::string& data) {
    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1(reinterpret_cast<const unsigned char*>(data.c_str()), data.size(), hash);
    
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (int i = 0; i < SHA_DIGEST_LENGTH; ++i) {
        oss << std::setw(2) << static_cast<int>(hash[i]);
    }
    return oss.str();
}

/**
 * @brief Writes data to a git object, compressing and storing it.
 *
 * Formats the data as "<type> <size>\0<data>", computes its SHA-1 hash,
 * compresses it with zlib, and writes it to the .tinygit/objects directory.
 * @param data The raw data to store (e.g., file content, commit info).
 * @param type The type of git object ("blob", "commit", "tree").
 * @return The SHA-1 hash of the created object.
 */
std::string write_object(const std::string& data, const std::string& type) {
    // Prepare the git object header and content
    std::string store = type + " " + std::to_string(data.size()) + '\0' + data;
    std::string sha = sha1(store);

    // Compress the data using zlib's one-shot compress function
    uLong bound = compressBound(store.size());
    std::vector<Bytef> compressed(bound);
    uLong compressed_len = bound;
    if (compress(compressed.data(), &compressed_len, reinterpret_cast<const Bytef*>(store.data()), store.size()) != Z_OK) {
        std::cerr << "Compression failed!" << std::endl;
        return "";
    }

    // Write the compressed data to the object database
    fs::path file(".tinygit/objects/" + sha.substr(0, 2) + "/" + sha.substr(2));
    fs::create_directories(file.parent_path());
    std::ofstream out(file, std::ios::binary);
    out.write(reinterpret_cast<const char*>(compressed.data()), compressed_len);
    
    return sha;
}

/**
 * @brief Reads data from a git object, decompressing it.
 * @param sha The SHA-1 hash of the object to read.
 * @return The decompressed, raw data from the object.
 */
std::string read_object(const std::string& sha) {
    fs::path file(".tinygit/objects/" + sha.substr(0, 2) + "/" + sha.substr(2));
    if (!fs::exists(file)) {
        return "";
    }
    
    std::ifstream in(file, std::ios::binary);
    std::vector<char> compressed((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    
    // Decompress the data using zlib's one-shot uncompress function
    // We start with an initial buffer size and double it if decompression fails
    // due to an insufficient buffer (Z_BUF_ERROR).
    uLong compressed_size = compressed.size();
    uLong decompressed_size = compressed_size * 5; // Start with a reasonable guess
    std::vector<Bytef> decompressed(decompressed_size);
    
    int ret;
    while ((ret = uncompress(decompressed.data(), &decompressed_size,
                             reinterpret_cast<const Bytef*>(compressed.data()), compressed_size)) == Z_BUF_ERROR) {
        decompressed_size *= 2;
        decompressed.resize(decompressed_size);
    }
    
    if (ret != Z_OK) {
        std::cerr << "Decompression failed!" << std::endl;
        return "";
    }
    
    std::string store(reinterpret_cast<char*>(decompressed.data()), decompressed_size);
    
    // Extract data by skipping the "type size\0" header
    size_t null_pos = store.find('\0');
    if (null_pos == std::string::npos) {
        return "";
    }
    
    return store.substr(null_pos + 1);
}

/**
 * @brief Main entry point for the tinygit command-line tool.
 */
int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "usage: tinygit <command> [<args>...]\n";
        return 1;
    }
    std::string cmd = argv[1];

    if (cmd == "init") {
        fs::create_directories(".tinygit/objects");
        fs::create_directories(".tinygit/refs/heads");
        std::ofstream(".tinygit/HEAD") << "ref: refs/heads/master\n";
        std::cout << "Initialized empty tinygit repository in ./.tinygit/\n";
    }
    else if (cmd == "add" && argc == 3) {
        std::string file_path = argv[2];
        std::ifstream in(file_path, std::ios::binary);
        if (!in) {
            std::cerr << "fatal: could not open file '" << file_path << "'\n";
            return 1;
        }
        std::string content((std::istreambuf_iterator<char>(in)),
                             std::istreambuf_iterator<char>());
        std::string sha = write_object(content, "blob");
        std::cout << "Staged " << file_path << " as blob " << sha << "\n";
    }
    else if (cmd == "commit" && argc == 4 && std::string(argv[2]) == "-m") {
        // NOTE: This is a simplified commit that doesn't build a real tree object.
        // It's for demonstration purposes.
        std::string message = argv[3];
        
        // 1. Create a placeholder tree object (in a real git, this would be built from the index)
        std::string tree_sha = "dummy_tree_sha_placeholder"; // In a real implementation, you'd build a tree object
        
        // 2. Create the commit object
        std::ostringstream commit;
        auto t = std::time(nullptr);
        commit << "tree " << tree_sha << "\n"
               << "author Your Name <you@example.com> " << t << " +0000\n"
               << "committer Your Name <you@example.com> " << t << " +0000\n\n"
               << message << "\n";
        std::string commit_sha = write_object(commit.str(), "commit");

        // 3. Update the current branch reference (e.g., master) to point to the new commit
        std::ofstream(".tinygit/refs/heads/master") << commit_sha << "\n";
        std::cout << "[master " << commit_sha.substr(0, 7) << "] " << message << "\n";
    }
    else if (cmd == "log") {
        std::ifstream head(".tinygit/refs/heads/master");
        std::string sha;
        if (head >> sha) {
            std::string commit_data = read_object(sha);
            std::cout << "commit " << sha << "\n" << commit_data << "\n";
        } else {
             std::cerr << "fatal: no commits yet\n";
        }
    }
    else {
        std::cerr << "tinygit: '" << cmd << "' is not a tinygit command.\n";
    }

    return 0;
}
```
