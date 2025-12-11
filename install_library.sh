#!/bin/bash

set -e

DOWNLOAD_FOLDER="/temp/download_packages"
mkdir -p "$DOWNLOAD_FOLDER"

# If INSTALL_FOLDER is not set -> use default /usr/local
if [ -z "$INSTALL_FOLDER" ]; then
    INSTALL_FOLDER="/usr/local"
    echo "INSTALL_FOLDER is not set. Using default: $INSTALL_FOLDER"
else
    echo "INSTALL_FOLDER = $INSTALL_FOLDER"
fi

# Create the directory if it does not exist
mkdir -p "$INSTALL_FOLDER"

# Change directory into INSTALL_FOLDER
cd "$INSTALL_FOLDER" || {
    echo "ERROR: Cannot cd into $INSTALL_FOLDER"
    exit 1
}

echo "Current directory: $(pwd)"

install_dependency_packages() {
    # -----------------------------------------
    # Skip installation when running in CI
    # -----------------------------------------
    if [[ "$IS_RUNNING_CI" == "1" ]]; then
        echo "CI mode: skip installing dependency packages"
        return
    fi

    apt-get update -y
    apt-get install -y wget curl git
    apt-get install -y build-essential
    apt-get install -y libgflags-dev

    apt-get install libmongoc-1.0-0 -y
    apt-get install libbson-1.0-0 -y
    apt-get install cmake libssl-dev libsasl2-dev -y
    cmake --version

    # Install Boost
    apt update -y
    apt install -y libboost-all-dev

    # Install SpdLog
    apt update -y
    apt install -y libspdlog-dev

    # Install PM2
    curl -fsSL https://deb.nodesource.com/setup_lts.x | bash -
    apt install -y nodejs
    node -v
    npm -v
    npm install -g pm2
    pm2 -v
}

install_dependency_packages

# Install the mongoc driver
if [ ! -f "$INSTALL_FOLDER/lib/libmongoc-1.0.so" ]; then
    cd "$DOWNLOAD_FOLDER"
    wget https://github.com/mongodb/mongo-c-driver/releases/download/1.23.0/mongo-c-driver-1.23.0.tar.gz
    tar xzf mongo-c-driver-1.23.0.tar.gz
    cd "$DOWNLOAD_FOLDER"/mongo-c-driver-1.23.0
    mkdir cmake-build
    cd "$DOWNLOAD_FOLDER"/mongo-c-driver-1.23.0/cmake-build
    pwd
    cmake -DCMAKE_INSTALL_PREFIX="$INSTALL_FOLDER" ..
    cmake -DENABLE_AUTOMATIC_INIT_AND_CLEANUP=OFF ..
    cmake -DMONGOC_TEST_USE_CRYPT_SHARED=OFF ..
    cmake --build . -j 4
    cmake --build . --target install
else
    echo "[Cache Hit] mongo-c-driver found -> skipping build."
fi

# Install the mongocxx driver
if [ ! -f "$INSTALL_FOLDER/lib/libmongocxx.so" ]; then
    cd "$DOWNLOAD_FOLDER"
    curl -OL https://github.com/mongodb/mongo-cxx-driver/releases/download/r3.6.7/mongo-cxx-driver-r3.6.7.tar.gz
    tar -xzf mongo-cxx-driver-r3.6.7.tar.gz
    # Patch all .hpp files that use std::uintXX_t or std::intXX_t but are missing <cstdint> (not nice, but works)
    find "$DOWNLOAD_FOLDER"/mongo-cxx-driver-r3.6.7/src -name "*.hpp" | while read file; do
    if grep -qE 'std::(u?int(8|16|32|64)_t)' "$file" && ! grep -q '<cstdint>' "$file"; then
        echo "Patching $file"
        sed -i '/#pragma once/a #include <cstdint>' "$file"
    fi
    done
    cd "$DOWNLOAD_FOLDER"/mongo-cxx-driver-r3.6.7/build
    cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$INSTALL_FOLDER"
    cmake --build . -j 4
    cmake --build . --target install
else
    echo "[Cache Hit] mongo-cxx-driver found -> skipping build."
fi

# Install google test
if [ -f "$INSTALL_FOLDER/lib/libgtest.a" ] || [ -d "$INSTALL_FOLDER/include/gtest" ]; then
    echo "[Cache Hit] GoogleTest already installed → skipping build."
else
    cd "$DOWNLOAD_FOLDER"
    git clone https://github.com/google/googletest.git
    cd "$DOWNLOAD_FOLDER"/googletest
    cmake -B build -DCMAKE_INSTALL_PREFIX="$INSTALL_FOLDER"
    cmake --build build
    cmake --install build
fi

# # Install GLog
# cd "$DOWNLOAD_FOLDER"
# git clone https://github.com/google/glog.git
# cd "$DOWNLOAD_FOLDER"/glog
# mkdir build && cd "$DOWNLOAD_FOLDER"/build
# cmake -DCMAKE_INSTALL_PREFIX="$INSTALL_FOLDER" ..
# make -j$(nproc) && make install

# Clean up if you want to keep the image size smaller
cd "$DOWNLOAD_FOLDER"
rm -rf glog
rm -rf boost_1_71_0*
rm -rf mongo-c*
rm -rf googletest

# Add Path
export LD_LIBRARY_PATH="$INSTALL_FOLDER/lib:$LD_LIBRARY_PATH"

find $INSTALL_FOLDER -name mongocxxConfig.cmake