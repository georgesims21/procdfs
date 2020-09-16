#!/bin/bash
cmake .
./procsys -d -f -s /home/vagrant/procsys 2 1234 eth1 /home/vagrant/shared/procsys/iplist.txt
