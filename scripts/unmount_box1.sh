#!/bin/bash
mount="/home/vagrant/shared/procsys/build_box1/mountdir"

if grep -qs "$mount" /proc/mounts; then
  fusermount "$mount" -u  2>&1
else
    exit 0
fi
