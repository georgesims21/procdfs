#!/bin/bash

build="/home/vagrant/shared/procdfs/scripts/build_box1.sh"
unmount="/home/vagrant/shared/procdfs/scripts/unmount_box1.sh"
procdfs="/home/vagrant/shared/procdfs/build_box1"

sh $build
sh $unmount
cd $procdfs
./procdfs -d -f -s /home/vagrant/mountdir 2 1234 enp0s8 /home/vagrant/shared/procdfs/iplists/host-1client.txt
