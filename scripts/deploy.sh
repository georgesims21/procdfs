#!/bin/bash
prun -np $1 -reserve $2 ./run.sh $1
