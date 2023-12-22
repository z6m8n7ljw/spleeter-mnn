#!/bin/bash
clear
set -e
set +x

rm -rf build
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j4

./spleeter-cpp