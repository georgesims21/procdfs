#!/bin/bash
mount2="/procsys2"

if grep -qs "$mount2" /proc/mounts; then
  fusermount -u ~/procsys2 2>&1
else
    exit 0
fi
