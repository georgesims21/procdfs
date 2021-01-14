#!/bin/bash
IP=$(ip -o -4 addr list ib0 | awk '{print $4}' | cut -d/ -f1)
fusermount -u "/home/gss680/mountdir$IP"
