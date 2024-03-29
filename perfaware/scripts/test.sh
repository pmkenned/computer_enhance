#!/bin/bash

#set -x
set -e

gcc -O2 -o sim8086.exe sim8086.c -lpthread
gcc -O2 -o gen_random_movs gen_random_movs.c

./gen_random_movs -i 100 > input/listing_1001_random.asm

FILES=(
    input/listing_0037_single_register_mov
    input/listing_0038_many_register_mov
    input/listing_0039_more_movs
    input/listing_0040_challenge_movs
#    input/listing_1000_all_repeated
    input/listing_1001_random
)

for INPUT in "${FILES[@]}"
do
    echo $INPUT
    nasm $INPUT.asm
    ./sim8086.exe $INPUT > output.asm
    nasm output.asm
    diff -q output $INPUT
done

rm output.asm output
echo "ALL TESTS PASSED"
