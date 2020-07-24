#!/bin/bash
mount="/procsys"

if grep -qs "$mount" /proc/mounts; then
  fusermount -u ~/procsys 2>&1
fi
