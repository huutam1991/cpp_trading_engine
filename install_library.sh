#!/bin/bash

install_dependency_packages() {
    # -----------------------------------------
    # Skip installation when running in CI
    # -----------------------------------------
    if [[ "$IS_RUNNING_CI" == "1" ]]; then
        echo "CI mode: skip installing dependency packages [$pkg]"
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
    curl -fsSL https://deb.nodesource.com/setup_lts.x | sudo -E bash -
    apt install -y nodejs
    node -v
    npm -v
    sudo npm install -g pm2
    pm2 -v
}

install_dependency_packages

cd /
wget https://github.com/mongodb/mongo-c-driver/releases/download/1.23.0/mongo-c-driver-1.23.0.tar.gz
tar xzf mongo-c-driver-1.23.0.tar.gz
cd /mongo-c-driver-1.23.0
mkdir cmake-build
cd /mongo-c-driver-1.23.0/cmake-build
pwd
cmake -DENABLE_AUTOMATIC_INIT_AND_CLEANUP=OFF ..
cmake -DMONGOC_TEST_USE_CRYPT_SHARED=OFF ..
cmake --build . -j 4
cmake --build . --target install

# Install the mongocxx driver
cd /
curl -OL https://github.com/mongodb/mongo-cxx-driver/releases/download/r3.6.7/mongo-cxx-driver-r3.6.7.tar.gz
tar -xzf mongo-cxx-driver-r3.6.7.tar.gz
# Patch all .hpp files that use std::uintXX_t or std::intXX_t but are missing <cstdint> (not nice, but works)
find /mongo-cxx-driver-r3.6.7/src -name "*.hpp" | while read file; do
  if grep -qE 'std::(u?int(8|16|32|64)_t)' "$file" && ! grep -q '<cstdint>' "$file"; then
    echo "Patching $file"
    sed -i '/#pragma once/a #include <cstdint>' "$file"
  fi
done
cd /mongo-cxx-driver-r3.6.7/build
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --build . -j 4
cmake --build . --target install

# Install google test
git clone https://github.com/google/googletest.git
cd googletest
cmake -B build -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --build build
cmake --install build

# # Install GLog
# cd /
# git clone https://github.com/google/glog.git
# cd glog
# mkdir build && cd build
# cmake -DCMAKE_INSTALL_PREFIX=/usr/local ..
# make -j$(nproc) && make install

# Clean up if you want to keep the image size smaller
cd /
rm -rf /glog
rm -rf /boost_1_71_0*
rm -rf /mongo-c*
rm -rf googletest/

# Add Path
export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH