#!/bin/zsh

mkdir -p "$HOME/vagrant/shared/procsys/build_host/mountdir"
cd "$HOME/vagrant/shared/procsys/build_host"
cmake ..
make -j
