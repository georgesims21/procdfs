#!/bin/bash
gcc src/server_new.c src/ds_new.c -lpthread -lm -o server_new_vm
./server_new_vm 3 1234 eth1 iplist.txt
