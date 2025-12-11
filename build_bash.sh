#!/bin/bash

mkdir build
cd build/
cmake .. \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_COMPILER=g++ \
  -DCMAKE_C_COMPILER=gcc \
  -DCMAKE_CXX_FLAGS="-O3 -march=native -mtune=native -flto -ffast-math -fno-plt" \
  -DCMAKE_C_FLAGS="-O3 -march=native -mtune=native -flto -ffast-math -fno-plt" \
  -DENABLE_UNITY_BUILD=OFF \
  -DBUILD_ONLY="core;sts;identitystore;s3;ec2"
cmake --build . -j 6
make -j
chmod 777 http_server_cpp
cp http_server_cpp ../