#!/bin/bash
./client_unmount.sh
./procsys -d -f -s /home/vagrant/procsys 3 1234 enp0s8 /home/vagrant/shared/procsys/iplist3.txt
