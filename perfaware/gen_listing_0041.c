#include <stdio.h>
#include <stdlib.h>

#define NELEMS(X) (sizeof(X)/sizeof(X[1]))

char * instructions[] = {
    "mov cx, bx",
    "mov ch, ah",
    "mov dx, bx",
    "mov si, bx",
    "mov bx, di",
    "mov al, cl",
    "mov ch, ch",
    "mov bx, ax",
    "mov bx, si",
    "mov sp, di",
    "mov bp, ax",

    // Register-to-register
    "mov si, bx",
    "mov dh, al",

    // 8-bit immediate-to-register
    "mov cl, 12",
    "mov ch, -12",

    // 16-bit immediate-to-register
    "mov cx, 12",
    "mov cx, -12",
    "mov dx, 3948",
    "mov dx, -3948",

    // Source address calculation
    "mov al, [bx + si]",
    "mov bx, [bp + di]",
    "mov dx, [bp]",

    // Source address calculation plus 8-bit displacement
    "mov ah, [bx + si + 4]",

    // Source address calculation plus 16-bit displacement
    "mov al, [bx + si + 4999]",

    // Dest address calculation
    "mov [bx + di], cx",
    "mov [bp + si], cl",
    "mov [bp], ch",

    // Signed displacements
    "mov ax, [bx + di - 37]",
    "mov [si - 300], cx",
    "mov dx, [bx - 32]",

    // Explicit sizes
    "mov [bp + di], byte 7",
    "mov [di + 901], word 347",

    // Direct address
    "mov bp, [5]",
    "mov bx, [3458]",

    // Memory-to-accumulator test
    "mov ax, [2555]",
    "mov ax, [16]",

    // Accumulator-to-memory test
    "mov [2554], ax",
    "mov [15], ax",
};

int main(int argc, char * argv[])
{
    long n = 10;
    if (argc > 1) {
        n = strtol(argv[1], NULL, 10);
    }

    printf("bits 16\n");
    for (int i = 0; i < n; i++) {
        printf("%s\n", instructions[rand() % NELEMS(instructions)]);
    }
}
