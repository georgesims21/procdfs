#!/bin/bash
proj="/home/gss680/procdfs"
nodes=$1
resnr=$2

rm -r "$proj"/build
mkdir "$proj"/build
cd "$proj"/build
cmake ..
make -j
cp "$proj"/run.sh .
cp "$proj"/drun.sh .
cp "$proj"/getip.sh .
cp "$proj"/kill.sh .

prun -np $nodes -reserve $resnr "$proj"/getip.sh
prun -np $nodes -reserve $resnr "$proj"/run.sh $nodes
