#!/bin/bash

mkdir -p /home/vagrant/mountdir
mkdir -p /home/vagrant/shared/procdfs/build_box1
cd /home/vagrant/shared/procdfs/build_box1
../scripts/unmount_box1.sh

cmake ..
make -j
