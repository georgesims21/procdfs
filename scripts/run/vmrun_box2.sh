#!/bin/bash

build="/home/vagrant/shared/procsys/scripts/build_box2.sh"
unmount="/home/vagrant/shared/procsys/scripts/unmount_box2.sh"
procsys="/home/vagrant/shared/procsys/build_box1/procsys"

sh $build
sh $unmount
$procsys -d -f -s /home/vagrant/shared/procsys/build_box2/mountdir 2 1234 enp0s8 /home/vagrant/shared/procsys/iplists/2client.txt
