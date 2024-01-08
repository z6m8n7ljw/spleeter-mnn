#!/bin/bash
clear
set -e
set +x

rm -rf build
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Debug -DENABLE_TESTING=1 ../test/
make -j8

git_root=$(git rev-parse --show-toplevel)
./test-audio-separation $git_root/audio/coc_f32.pcm $git_root/models/vocal.mnn $git_root/models/accompaniment.mnn