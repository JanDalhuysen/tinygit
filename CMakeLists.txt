cmake_minimum_required(VERSION 3.10)

project(tinygit)

# Link against libcrypto and zlib
find_package(ZLIB REQUIRED)
find_package(OpenSSL REQUIRED)

# target_link_libraries(main PRIVATE OpenSSL::SSL)
# target_link_libraries(main PRIVATE OpenSSL::Crypto)
# target_link_libraries(main PRIVATE ZLIB::ZLIB)

add_executable(tinygit1 tinygit1.cpp)
add_executable(tinygit2 tinygit2.cpp)
add_executable(tinygit combined_tinygit.cpp)

target_link_libraries(tinygit
    OpenSSL::Crypto
    ZLIB::ZLIB
)

target_link_libraries(tinygit1
    OpenSSL::Crypto
    ZLIB::ZLIB
)

target_link_libraries(tinygit2
    OpenSSL::Crypto
    ZLIB::ZLIB
)
