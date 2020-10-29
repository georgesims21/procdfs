#!/bin/bash
mount="/home/vagrant/mountdir"

if grep -qs "$mount" /proc/mounts; then
  fusermount -u "$mount" 2>&1
else
    exit 0
fi
