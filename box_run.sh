#!/bin/bash
prog="procdfs"
proj="/home/vagrant/shared/$prog"
name=$1
ip=$2

unmount="$proj/box_kill.sh"
procdfs="$proj/$name"

cd "$proj"
rm -r "$name"
mkdir -p "$name"
cd "$name"
sh "$unmount"
set -e
cmake ..
make -j

cd "$procdfs"
./procdfs -d "/home/vagrant/$prog" 2 1234 enp0s8 "$proj/iplists/2client.txt" "$ip"
