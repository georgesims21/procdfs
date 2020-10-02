#!/bin/bash

mkdir -p "$HOME/shared/procsys2/build_client/mountdir"
cd "$HOME/shared/procsys2/build_client"
cmake ..
make -j