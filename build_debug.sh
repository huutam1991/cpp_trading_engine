#!/bin/bash

mkdir debug
cd debug/
cmake .. -DENABLE_UNITY_BUILD=OFF -DBUILD_ONLY="core;sts;identitystore;s3;ec2" -DCMAKE_BUILD_TYPE=Debug
cmake --build . -j 4
make -j
chmod 777 http_server_cpp
cp http_server_cpp ../http_server_cpp_debug