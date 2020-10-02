#!/bin/bash
mount="/home/george/vagrant/shared/procsys/build_host/mountdir"

if grep -qs "$mount" /proc/mounts; then
  fusermount -u "$mount" 2>&1
else
    exit 0
fi
