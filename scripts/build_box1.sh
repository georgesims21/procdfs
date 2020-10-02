#!/bin/bash

mkdir -p /home/vagrant/shared/procsys/build_box1/mountdir
cd /home/vagrant/shared/procsys/build_box1
pwd
../scripts/unmount_box1.sh

cmake ..
make -j
