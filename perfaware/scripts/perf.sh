#!/bin/bash

#set -x
set -e

gcc -O2 -o sim8086.exe sim8086.c -lpthread
gcc -O2 -o gen_random_movs gen_random_movs.c

INPUT=input/listing_0042_random

SIZE=1024
for i in $(seq 7); do
    ./gen_random_movs -b $SIZE > $INPUT.asm
    nasm $INPUT.asm
    ./sim8086.exe $INPUT > output.asm
    #nasm output.asm
    #diff -q output $INPUT
    SIZE=$(( 2*SIZE ))
done

rm -f output.asm output
