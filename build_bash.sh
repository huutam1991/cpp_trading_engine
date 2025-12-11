#!/bin/bash

find $INSTALL_FOLDER -name mongocxxConfig.cmake

mkdir build
cd build/
cmake .. -DCMAKE_CXX_COMPILER=g++ -DCMAKE_C_COMPILER=gcc -DENABLE_UNITY_BUILD=OFF -DBUILD_ONLY="core;sts;identitystore;s3;ec2"
cmake --build . -j 6
make -j
chmod 777 http_server_cpp
cp http_server_cpp ../