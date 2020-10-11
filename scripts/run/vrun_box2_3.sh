#!/bin/bash

build="/home/vagrant/shared/procsys/scripts/build_box2.sh"
unmount="/home/vagrant/shared/procsys/scripts/unmount_box2.sh"
procsys="/home/vagrant/shared/procsys/build_box2/procsys"

sh $build
sh $unmount
valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes -v $procsys -d -f -s /home/vagrant/shared/procsys/build_box2/mountdir 3 1234 enp0s8 /home/vagrant/shared/procsys/iplists/host-2client.txt
