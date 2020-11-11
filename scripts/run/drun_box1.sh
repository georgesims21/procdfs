#!/bin/bash

build="/home/vagrant/shared/procdfs/scripts/build_box1.sh"
unmount="/home/vagrant/shared/procdfs/scripts/unmount_box1.sh"
procdfs="/home/vagrant/shared/procdfs/build_box1"

sh $build
sh $unmount
cd $procdfs
gdb ./procdfs
	-q \
	-batch \
	-ex 'set print thread-events off' \
	-ex 'handle SIGALRM nostop pass' \
	-ex 'handle SIGCHLD nostop pass' \
#	-ex 'break fileops_new.c:169' \

#-ex 'run' \

#-ex 'thread apply all backtrace' \

#--args \
#    ./procdfs -d -f -s /home/vagrant/shared/procdfs/build_box1/mountdir 2 1234 enp0s8 /home/vagrant/shared/procdfs/iplists/host-1client.txt
