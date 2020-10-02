#!/bin/bash

mkdir -p /home/vagrant/shared/procsys/build_box2/mountdir
cd /home/vagrant/shared/procsys/build_box2
pwd
../scripts/unmount_box2.sh

cmake ..
make -j
