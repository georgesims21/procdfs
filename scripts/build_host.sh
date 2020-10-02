#!/bin/zsh

mkdir -p "$HOME/vagrant/shared/procsys2/build_host/mountdir"
cd "$HOME/vagrant/shared/procsys2/build_host"
cmake ..
make -j