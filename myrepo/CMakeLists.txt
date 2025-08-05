cmake_minimum_required(VERSION 3.10)

project(tinygit)

find_package(ZLIB REQUIRED)
find_package(OpenSSL REQUIRED)


# target_link_libraries(main PRIVATE OpenSSL::SSL)
# target_link_libraries(main PRIVATE OpenSSL::Crypto)
# target_link_libraries(main PRIVATE ZLIB::ZLIB)

add_executable(tinygit tinygit.cpp)

# Link against libcrypto and zlib
# find_package(OpenSSL REQUIRED)
# find_package(ZLIB REQUIRED)

target_link_libraries(tinygit
    OpenSSL::Crypto
    ZLIB::ZLIB
)
