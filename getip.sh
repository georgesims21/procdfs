#!/bin/bash
IP=$(ip -o -4 addr list ib0 | awk '{print $4}' | cut -d/ -f1)
set IP
echo "$IP" >> iplist.txt
