#!/bin/bash
./client_unmount.sh
clang -Wall -g src/main.c src/server_new.c src/ds_new.c -o procsys -lm -lpthread `pkg-config fuse3 --cflags --libs`
#gdb --args ./procsys -d -f -s /home/vagrant/procsys 2 1234 enp0s8 /home/vagrant/shared/procsys/iplist2.txt

gdb -q \
	-batch \
	-ex 'set print thread-events off' \
        -ex 'handle SIGALRM nostop pass' \
        -ex 'handle SIGCHLD nostop pass' \
        -ex 'run' \
        -ex 'thread apply all backtrace' \
	--args \
	./procsys \
	-d -f -s /home/vagrant/procsys 2 1234 enp0s8 /home/vagrant/shared/procsys/iplist.txt

