#!/bin/bash

build="/home/vagrant/shared/procsys/scripts/build_box2.sh"
unmount="/home/vagrant/shared/procsys/scripts/unmount_box2.sh"
procsys="/home/vagrant/shared/procsys/build_box2"

sh $build
sh $unmount
cd $procsys
gdb -q \
    -batch \
    -ex 'set print thread-events off' \
    -ex 'handle SIGALRM nostop pass' \
    -ex 'handle SIGCHLD nostop pass' \
    -ex 'run' \
    -ex 'thread apply all backtrace' \
    --args \
    ./procsys \
    -d -f -s /home/vagrant/shared/procsys/build_box2/mountdir 3 1234 enp0s8 /home/vagrant/shared/procsys/iplists/host-2client.txt
