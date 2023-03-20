#!/bin/bash

N="${1:-1}"

rm -f listing_0041_all_repeated.asm
for i in $(seq $N); do
    echo "; ====" >> listing_0041_all_repeated.asm
    cat listing_0037_single_register_mov.asm listing_0038_many_register_mov.asm listing_0039_more_movs.asm listing_0040_challenge_movs.asm >> listing_0041_all_repeated.asm
done
