#!/bin/bash
pid=$(ps -A | grep procdfs | awk '{print $1}')
IP=$(ip -o -4 addr list ib0 | awk '{print $4}' | cut -d/ -f1)

if [ $pid > 0 ] 
then
    echo "ok"
    kill -9 $pid
else
    echo "no"
fi
fusermount -u "/home/gss680/mountdir$IP"
