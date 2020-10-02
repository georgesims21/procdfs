#!/bin/bash

mkdir -p /home/vagrant/shared/procsys2/build_box2/mountdir
cd /home/vagrant/shared/procsys2/build_box2
../scripts/client_unmount.sh

cmake ..
make -j
./procsys -d -f -s /home/vagrant/shared/procsys2/build_box2/mountdir 2 1234 enp0s8 /home/vagrant/shared/procsys2/iplists/2client.txt
