#!/bin/bash
clear
set -e
set +x

rm -rf build
mkdir build
cd build
cmake ..
make -j8

git_root=$(git rev-parse --show-toplevel)
./spleeter-cpp $git_root/audio/coc.wav