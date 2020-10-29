#!/bin/bash

mkdir -p /home/vagrant/mountdir
cd /home/vagrant/shared/procsys/build_box2
../scripts/unmount_box2.sh

cmake ..
make -j
