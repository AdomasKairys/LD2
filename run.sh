#!/bin/sh
re='^[0-9]+$'
if [[ $# != 1 || ($# > 0 && $1 =~ $re)]]; then
    echo "One argument is req and the argument must be a number"
fi
mpic++ main.cpp
mpirun -np $1 ./a.out