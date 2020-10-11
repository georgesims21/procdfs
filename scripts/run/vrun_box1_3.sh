#!/bin/bash

build="/home/vagrant/shared/procsys/scripts/build_box1.sh"
unmount="/home/vagrant/shared/procsys/scripts/unmount_box1.sh"
procsys="/home/vagrant/shared/procsys/build_box1/procsys"

sh $build
sh $unmount
valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes -v $procsys -d -f -s /home/vagrant/shared/procsys/build_box1/mountdir 3 1234 enp0s8 /home/vagrant/shared/procsys/iplists/host-2client.txt
