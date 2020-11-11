#!/bin/zsh

mkdir -p "$HOME/vagrant/shared/procdfs/build_host/mountdir"
cd "$HOME/vagrant/shared/procdfs/build_host"
cmake ..
make -j
