#!/bin/bash
rm -r build
mkdir build
cd build
mkdir mountdir
cmake ..
make -j
cp ../run.sh .
cp ../drun.sh .
cp ../getip.sh .
cp ../unmount.sh .
