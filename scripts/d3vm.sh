#!/bin/bash
cmake .
make -j
gdb ./server_new 3 1234 enp0s8 iplist3.txt
