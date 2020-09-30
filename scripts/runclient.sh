#!/bin/bash
cp CMakeCache_client.txt CMakeCache.txt
cmake .
make -j
./d2vm.sh
