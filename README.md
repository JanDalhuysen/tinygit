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

