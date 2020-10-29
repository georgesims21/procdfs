#!/bin/bash

build="/home/vagrant/shared/procsys/scripts/build_box2.sh"
unmount="/home/vagrant/shared/procsys/scripts/unmount_box2.sh"
procsys="/home/vagrant/shared/procsys/build_box2"

sh $build
sh $unmount
cd $procsys
./procsys -d -f -s /home/vagrant/mountdir 3 1234 enp0s8 /home/vagrant/shared/procsys/iplists/host-2client.txt
