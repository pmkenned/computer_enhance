#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#define NELEMS(X) (sizeof(X)/sizeof(X[1]))

typedef struct {
    char * str;
    size_t size;
} InstrData;

InstrData instructions[] = {
    { "mov cx, bx", 2},
    { "mov ch, ah", 2},
    { "mov dx, bx", 2},
    { "mov si, bx", 2},
    { "mov bx, di", 2},
    { "mov al, cl", 2},
    { "mov ch, ch", 2},
    { "mov bx, ax", 2},
    { "mov bx, si", 2},
    { "mov sp, di", 2},
    { "mov bp, ax", 2},

    // Register-to-register
    { "mov si, bx", 2},
    { "mov dh, al", 2},

    // 8-bit immediate-to-register
    { "mov cl, 12", 2},
    { "mov ch, -12", 2},

    // 16-bit immediate-to-register
    { "mov cx, 12", 3},
    { "mov cx, -12", 3},
    { "mov dx, 3948", 3},
    { "mov dx, -3948", 3},

    // Source address calculation
    { "mov al, [bx + si]", 2},
    { "mov bx, [bp + di]", 2},
    { "mov dx, [bp]", 3},

    // Source address calculation plus 8-bit displacement
    { "mov ah, [bx + si + 4]", 3},

    // Source address calculation plus 16-bit displacement
    { "mov al, [bx + si + 4999]", 4},

    // Dest address calculation
    { "mov [bx + di], cx", 2},
    { "mov [bp + si], cl", 2},
    { "mov [bp], ch", 3},

    // Signed displacements
    { "mov ax, [bx + di - 37]", 3},
    { "mov [si - 300], cx", 4},
    { "mov dx, [bx - 32]", 3},

    // Explicit sizes
    { "mov [bp + di], byte 7", 3},
    { "mov [di + 901], word 347", 6},

    // Direct address
    { "mov bp, [5]", 4},
    { "mov bx, [3458]", 4},

    // Memory-to-accumulator test
    { "mov ax, [2555]", 3},
    { "mov ax, [16]", 3},

    // Accumulator-to-memory test
    { "mov [2554], ax", 3},
    { "mov [15], ax", 3},

    // Immediate to direct address
    { "mov [2554], byte 10", 5},
    { "mov [15], word 300", 6},
};

enum {
    MODE_BYTES,
    MODE_INSTRS,
};

const char * program_name;

static void
usage()
{
    fprintf(stderr, "usage: %s -i N or\n"
                    "       %s -b N\n"
                    "  -i N will print N instructions\n"
                    "  -b N will generate a program of (about) N bytes\n", program_name, program_name);
    exit(0);
}

int main(int argc, char * argv[])
{
    program_name = argv[0];
    long n = 10;
    int mode = MODE_INSTRS;
    if (argc == 1) {
    } else if (argc == 3) {
        if      (strcmp(argv[1], "-b") == 0)    mode = MODE_BYTES;
        else if (strcmp(argv[1], "-i") == 0)    mode = MODE_INSTRS;
        else                                    usage();
        n = strtol(argv[2], NULL, 10);
    } else {
        usage();
    }

    printf("bits 16\n");
    if (mode == MODE_INSTRS) {
        size_t nbytes = 0;
        for (int i = 0; i < n; i++) {
            InstrData instr = instructions[rand() % NELEMS(instructions)];
            nbytes += instr.size;
            printf("%s\n", instr.str);
        }
        //fprintf(stderr, "%zu\n", nbytes);
    } else {
        size_t nbytes = 0;
        while (nbytes < n) {
            InstrData instr = instructions[rand() % NELEMS(instructions)];
            nbytes += instr.size;
            printf("%s\n", instr.str);
        }
    }
}
