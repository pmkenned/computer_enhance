#!/bin/bash

N="${1:-1}"

rm -f input/listing_0041_all_repeated.asm
for i in $(seq $N); do
    echo "; ====" >> input/listing_0041_all_repeated.asm
    cat input/listing_0037_single_register_mov.asm  \
        input/listing_0038_many_register_mov.asm    \
        input/listing_0039_more_movs.asm            \
        input/listing_0040_challenge_movs.asm       >> input/listing_0041_all_repeated.asm
done
