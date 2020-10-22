#!/bin/bash
dir="/home/vagrant/shared/procsys/build_box1/mountdir/net"
for i in {1..50}
do
	   ls $dir &
	   cat "${dir}/dev"
   done
