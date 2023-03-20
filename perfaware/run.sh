#!/bin/bash

set -xe

INPUT="${1:-input/listing_0038_many_register_mov}"

gcc -o sim8086.exe sim8086.c
./sim8086.exe $INPUT > output.asm
cat output.asm
