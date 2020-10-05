#!/bin/bash

build="/home/vagrant/shared/procsys/scripts/build_box1.sh"
unmount="/home/vagrant/shared/procsys/scripts/unmount_box1.sh"
procsys="/home/vagrant/shared/procsys/build_box1/procsys"

sh $build
sh $unmount
gdb -ex 'break main.c:179' \
    -ex 'run' \
    --args \
    $procsys \
    -d -f -s /home/vagrant/shared/procsys/build_box1/mountdir 2 1234 enp0s8 /home/vagrant/shared/procsys/iplists/2client.txt
    #-q \
    #-batch \
    #-ex 'set print thread-events off' \
    #-ex 'handle SIGALRM nostop pass' \
    #-ex 'handle SIGCHLD nostop pass' \
    #-ex 'thread apply all backtrace' \
