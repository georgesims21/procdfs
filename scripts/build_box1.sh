#!/bin/bash

mkdir -p /home/vagrant/mountdir
cd /home/vagrant/shared/procsys/build_box1
../scripts/unmount_box1.sh

cmake ..
make -j
