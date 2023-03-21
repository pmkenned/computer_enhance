#!/bin/bash

set -xe

gcc -o sim8086.exe sim8086.c

FILES=(
    listing_0037_single_register_mov
    listing_0038_many_register_mov
    listing_0039_more_movs
    listing_0040_challenge_movs
)

for INPUT in "${FILES[@]}"
do
    ./sim8086.exe $INPUT > output.asm
    nasm output.asm
    diff -q output $INPUT
done

rm output.asm output
