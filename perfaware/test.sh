#!/bin/bash

set -xe

gcc -O2 -o sim8086.exe sim8086.c

FILES=(
#    listing_0037_single_register_mov
#    listing_0038_many_register_mov
#    listing_0039_more_movs
#    listing_0040_challenge_movs
    listing_0041_all_repeated
)

for INPUT in "${FILES[@]}"
do
    nasm $INPUT.asm
    ./sim8086.exe $INPUT > output.asm
    nasm output.asm
    diff -q output $INPUT
done

rm output.asm output
echo "ALL TESTS PASSED"
