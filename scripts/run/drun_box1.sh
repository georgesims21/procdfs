#!/bin/bash

build="/home/vagrant/shared/procsys/scripts/build_box1.sh"
unmount="/home/vagrant/shared/procsys/scripts/unmount_box1.sh"
procsys="/home/vagrant/shared/procsys/build_box1"

sh $build
sh $unmount
cd $procsys
gdb -q \
    -batch \
    -ex 'set print thread-events off' \
    -ex 'handle SIGALRM nostop pass' \
    -ex 'handle SIGCHLD nostop pass' \
    -ex 'break fileops_new.c:169' \
    -ex 'run' \
    -ex 'thread apply all backtrace' \
    --args \
    ./procsys \
    -d -f -s /home/vagrant/shared/procsys/build_box1/mountdir 2 1234 enp0s8 /home/vagrant/shared/procsys/iplists/host-1client.txt
