#!/bin/bash
IP=$(ip -o -4 addr list ib0 | awk '{print $4}' | cut -d/ -f1)
N=$1
MOUNT="/home/gss680/mountdir$IP"
mkdir $MOUNT
./procdfs $MOUNT $N 1234 ib0 iplist.txt "$IP"
