#!/bin/bash
re='^[0-9]+$'
if ! [[ $# -eq 1 ]] || [[ $# -gt 0 && ! $1 =~ $re ]]; then
    echo "One argument is req and the argument must be a number"
    exit
fi
mpic++ main.cpp Lifter.cpp SHA256.cpp
mpirun --oversubscribe -np $1 ./a.out