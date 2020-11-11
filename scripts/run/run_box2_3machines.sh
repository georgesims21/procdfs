#!/bin/bash

build="/home/vagrant/shared/procdfs/scripts/build_box2.sh"
unmount="/home/vagrant/shared/procdfs/scripts/unmount_box2.sh"
procdfs="/home/vagrant/shared/procdfs/build_box2"

sh $build
sh $unmount
cd $procdfs
./procdfs -d -f -s /home/vagrant/mountdir 3 1234 enp0s8 /home/vagrant/shared/procdfs/iplists/host-2client.txt
