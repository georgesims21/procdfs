#!/bin/bash

mkdir -p /home/vagrant/shared/procsys/build_box1/mountdir
cd /home/vagrant/shared/procsys/build_box1
pwd
../scripts/box1_unmount.sh

cmake ..
make -j
#./procsys -d -f -s /home/vagrant/shared/procsys/build_box1/mountdir 2 1234 enp0s8 /home/vagrant/shared/procsys/iplists/2client.txt
gdb --args ./procsys -d -f -s /home/vagrant/shared/procsys/build_box1/mountdir 2 1234 enp0s8 /home/vagrant/shared/procsys/iplists/host_1client.txt
