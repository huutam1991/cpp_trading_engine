#!/bin/bash

mkdir build
cd build/
cmake .. -DENABLE_UNITY_BUILD=OFF -DBUILD_ONLY="core;sts;identitystore;s3;ec2"
cmake --build . -j 6
make -j
chmod 777 http_server_cpp
cp http_server_cpp ../