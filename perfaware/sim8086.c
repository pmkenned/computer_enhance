#define _POSIX_C_SOURCE 199309L
#include "timer.h"
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>
#include <inttypes.h>
#include <assert.h>

static size_t
read_entire_file(const char * filename, char * buffer, size_t len)
{
    FILE * fp = fopen(filename, "r");
    if (fp == NULL) {
        perror(filename);
        exit(1);
    }
    size_t nread = fread(buffer, 1, len, fp);

    if (nread == len) {
        fprintf(stderr, "error: file too large\n");
        exit(1);
    }

    if (ferror(fp)) {
        perror(filename);
        exit(1);
    }

    fclose(fp);
    return nread;
}

char * reg_decode_table[] = {
    "al", "ax",
    "cl", "cx",
    "dl", "dx",
    "bl", "bx",
    "ah", "sp",
    "ch", "bp",
    "dh", "si",
    "bh", "di",
};

char * rm_decode_no_disp_table[] = {
    "[bx + si]",
    "[bx + di]",
    "[bp + si]",
    "[bp + di]",
    "[si]",
    "[di]",
    "[bp]",
    "[bx]",
};

char * rm_decode_table[] = {
    "[bx + si%s]",
    "[bx + di%s]",
    "[bp + si%s]",
    "[bp + di%s]",
    "[si%s]",
    "[di%s]",
    "[bp%s]",
    "[bx%s]",
};

enum {
    MOV_REG_MEM_TO_FROM_REG,
    MOV_IMM_TO_REG_MEM,
    MOV_IMM_TO_REG,
    MOV_MEM_TO_ACC,
    MOV_ACC_TO_MEM,
    MOV_REG_MEM_TO_SEG,
    MOV_SEG_TO_REG_MEM,
};

size_t nbytes_table[] = {
    [MOV_REG_MEM_TO_FROM_REG]   = 2,
    [MOV_IMM_TO_REG_MEM]        = 2,
    [MOV_IMM_TO_REG]            = 1,
    [MOV_MEM_TO_ACC]            = 3,
    [MOV_ACC_TO_MEM]            = 3,
    [MOV_REG_MEM_TO_SEG]        = 2,
    [MOV_SEG_TO_REG_MEM]        = 2,
};

// MOV_REG_MEM_TO_FROM_REG
//  mov cx, bx
//  mov ch, bh
//  mov cx, [bp + di]
//  mov ch, [bp + di]
//  mov cx, [bp]
//  mov cx, [bp + di + 16]
//  mov ch, [bp + di + 16]
//  mov cx, [bp + di - 16]
//  mov ch, [bp + di - 16]
//  mov cx, [bp + di + 5000]
//  mov ch, [bp + di + 5000]
//  mov [bp + di], cx
//  mov [bp + di], ch
//  mov [bp + di + 16], cx
//  mov [bp + di + 16], ch
//  mov [bp + di + 5000], cx
//  mov [bp + di + 5000], ch
// MOV_IMM_TO_REG_MEM
//  mov [bp + di], byte 32
//  mov [bp + di], word 32
//  mov [bp], word 32
//  mov [bp + di + 16], byte 32
//  mov [bp + di + 16], word 32
//  mov [bp + di - 16], byte 32
//  mov [bp + di - 16], word 32
//  mov [bp + di + 5000], byte 32
//  mov [bp + di + 5000], word 32
//  mov [16], byte 32
//  mov [16], word 32
//  mov [5000], byte 32
//  mov [5000], word 32
// MOV_IMM_TO_REG
//  mov cx, 32
//  mov ch, 32

static size_t
dasm_v1(char * bytes, char * output, size_t len)
{
    output[0] = '\0';
    size_t nbytes = 0;
    int nprint = 0;

    char disp_str[32] = "";
    char rm_str[32] = "";

    if ((bytes[0] & 0xfc) == 0x88) {
        // reg/mem to/from reg
        int d       = (bytes[0] >> 1) & 0x1;
        int w       = (bytes[0] >> 0) & 0x1;
        int mod     = (bytes[1] >> 6) & 0x3;
        int reg     = (bytes[1] >> 3) & 0x7;
        int rm      = (bytes[1] >> 0) & 0x7;

        char * reg_str    = reg_decode_table[reg*2 + w];
        char * rm_str_fmt = rm_decode_table[rm];

        char * dst = d ? reg_str : rm_str;
        char * src = d ? rm_str : reg_str;
        if (mod == 0) {
            if (rm == 6) {
                uint8_t disp_lo = bytes[2];
                uint8_t disp_hi = w ? bytes[3] : 0;
                uint16_t disp = disp_lo + (disp_hi << 8);
                snprintf(rm_str, sizeof(rm_str), "[%" PRIu16 "]", disp);
                nbytes = 4;
            } else {
                snprintf(rm_str, sizeof(rm_str), rm_str_fmt, disp_str);
                nbytes = 2;
            }
            nprint = snprintf(output, len, "mov %s, %s", dst, src);
        } else if (mod == 1) {
            // 1 byte disp
            int8_t disp_lo = bytes[2];
            snprintf(disp_str, sizeof(disp_str), " + %" PRId8, disp_lo);
            snprintf(rm_str, sizeof(rm_str), rm_str_fmt, disp_str);
            nprint = snprintf(output, len, "mov %s, %s", dst, src);
            nbytes = 3;
        } else if (mod == 2) {
            // 2 byte disp
            uint8_t disp_lo = bytes[2];
            uint8_t disp_hi = bytes[3];
            int16_t disp = disp_lo + (disp_hi << 8);
            snprintf(disp_str, sizeof(disp_str), " + %" PRId16, disp);
            snprintf(rm_str, sizeof(rm_str), rm_str_fmt, disp_str);
            nprint = snprintf(output, len, "mov %s, %s", dst, src);
            nbytes = 4;
        } else if (mod == 3) {
            char * rm_str  = reg_decode_table[rm*2 + w];
            dst = d ? reg_str : rm_str;
            src = d ? rm_str : reg_str;
            nprint = snprintf(output, len, "mov %s, %s", dst, src);
            nbytes = 2;
        }
    } else if ((bytes[0] & 0xfe) == 0xc6) {
        // imm to reg/mem
        int w       = (bytes[0] >> 0) & 0x1;
        int mod     = (bytes[1] >> 6) & 0x3;
        int rm      = (bytes[1] >> 0) & 0x7;
        char * rm_str_fmt = rm_decode_table[rm];
        uint8_t disp_lo = 0, disp_hi = 0;
        uint8_t data_lo = 0, data_hi = 0;
        if (mod == 0) {
            //assert(rm != 6);
            if (rm == 6) {
                // direct address
                disp_lo = bytes[2];
                disp_hi = bytes[3];
                data_lo = bytes[4];
                data_hi = w ? bytes[5] : 0;
                nbytes = w ? 6 : 5;
            } else {
                // no displacement
                data_lo = bytes[2];
                data_hi = w ? bytes[3] : 0;
                nbytes = w ? 4 : 3;
            }
        } else if (mod == 1) {
            // 1 byte disp
            disp_lo = bytes[2];
            disp_hi = (disp_lo & 0x80) ? 0xff : 0;
            data_lo = bytes[3];
            data_hi = w ? bytes[4] : 0;
            nbytes = w ? 5 : 4;
        } else if (mod == 2) {
            // 2 byte disp
            disp_lo = bytes[2];
            disp_hi = bytes[3];
            data_lo = bytes[4];
            data_hi = w ? bytes[5] : 0;
            nbytes = w ? 6 : 5;
        } else if (mod == 3) {
            assert(0);
        }
        uint16_t disp = disp_lo + (disp_hi << 8);
        uint16_t data = data_lo + (data_hi << 8);
        if (mod == 0 && rm == 6) {
            snprintf(rm_str, sizeof(rm_str), "[%" PRIu16 "]", disp);
        } else {
            snprintf(disp_str, sizeof(disp_str), " + %" PRId16, disp);
            snprintf(rm_str, sizeof(rm_str), rm_str_fmt, disp_str);
        }
        char * size = w ? "word" : "byte";
        nprint = snprintf(output, len, "mov %s, %s %" PRIu16, rm_str, size, data);
    } else if ((bytes[0] & 0xf0) == 0xb0) {
        // imm to reg
        int w   = (bytes[0] >> 3) & 1;
        int reg = (bytes[0] >> 0) & 7;
        char * dst = reg_decode_table[reg*2+w];
        char * size = w ? "word" : "byte";
        uint8_t data_lo = bytes[1];
        uint8_t data_hi = w ? bytes[2] : 0;
        nbytes = w ? 3 : 2;
        uint16_t imm = data_lo + (data_hi << 8);
        nprint = snprintf(output, len, "mov %s, %s %" PRIu16, dst, size, imm);
    } else if ((bytes[0] & 0xfe) == 0xa0) {
        // mem to ax
        int w = bytes[0] & 1;
        uint8_t addr_lo = bytes[1];
        uint8_t addr_hi = bytes[2];
        uint16_t addr = addr_lo + (addr_hi << 8);
        char * dst = w ? "ax" : "al";
        nbytes = 3;
        nprint = snprintf(output, len, "mov %s, [%" PRIu16 "]", dst, addr);
    } else if ((bytes[0] & 0xfe) == 0xa2) {
        // ax to mem
        int w = bytes[0] & 1;
        uint8_t addr_lo = bytes[1];
        uint8_t addr_hi = bytes[2];
        uint16_t addr = addr_lo + (addr_hi << 8);
        char * src = w ? "ax" : "al";
        nbytes = 3;
        nprint = snprintf(output, len, "mov [%" PRIu16 "], %s", addr, src);
    } else if ((bytes[0] & 0xff) == 0x8e) {
        assert(0);
    } else if ((bytes[0] & 0xff) == 0x8c) {
        assert(0);
    } else {
        fprintf(stderr, "%x\n", bytes[0]);
        assert(0);
    }

    assert(nprint < len);
    assert(nbytes > 0);
    return nbytes;
}

static uint32_t
ndigits10(uint64_t x)
{
    uint32_t result = 0;
    do {
        result++;
        x /= 10;
    } while (x);
    return result;
}

static int
sprint_int(char * s, int64_t x)
{
    int negative = 0;
    if (x < 0) {
        x *= -1;
        negative = 1;
        *s++ = '-';
    }
    int ndigits = ndigits10(x);
    for (int i = 1; i <= ndigits; i++) {
        int64_t y = x/10;
        s[ndigits-i] = (x - y*10) + '0';
        x = y;
    }
    s[ndigits] = '\0';
    return ndigits + negative;
}

static void
direct_address(char * rm_str, int addr)
{
    // rm_str = "[DISP]"
    rm_str[0] = '[';
    int nchars = sprint_int(rm_str+1, addr);
    rm_str[nchars+1] = ']';
    rm_str[nchars+2] = '\0';
}

static void
displacement(char * rm_str, int rm, int disp)
{
    // rm_str = "[... + DISP]"
    int rm_len = (rm < 4) ? 8 : 3;
    strcpy(rm_str, rm_decode_no_disp_table[rm]);
    strcpy(rm_str + rm_len, "+ ");
    int nchars = sprint_int(rm_str + rm_len + 2, disp);
    strcpy(rm_str + rm_len + 2 + nchars, "]");
}

static void
reg_mem_to_from_reg(char * output, char * dst, char * src)
{
    // output = "mov DST, SRC"
    memcpy(output, "mov ", 4);
    size_t dst_len = strlen(dst);
    strcpy(output+4, dst);
    strcpy(output+4+dst_len, ", ");
    strcpy(output+6+dst_len, src);
}

static void
imm_to_reg_mem(char * output, char * rm_str, int w, int data)
{
    // output = "mov [...], word DATA"
    char * size = w ? "word " : "byte ";
    memcpy(output, "mov ", 4);
    size_t rm_len = strlen(rm_str);
    strcpy(output + 4, rm_str);
    strcpy(output + 4 + rm_len, ", ");
    strcpy(output + 6 + rm_len, size);
    sprint_int(output + 11 + rm_len, data);
}

static void
imm_to_reg(char * output, char * dst, int data)
{
    // output = "mov REG, DATA"
    memcpy(output, "mov ", 4);
    size_t dst_len = strlen(dst);
    strcpy(output + 4, dst);
    strcpy(output + 4 + dst_len, ", ");
    sprint_int(output + 6 + dst_len, data);
}

static void
mem_to_acc(char * output, char * dst, int addr)
{
    // output = "mov ax, [ADDR]"
    memcpy(output, "mov ", 4);
    size_t dst_len = strlen(dst);
    strcpy(output + 4, dst);
    strcpy(output + 4 + dst_len, ", [");
    int nchars = sprint_int(output + 7 + dst_len, addr);
    strcpy(output + 7 + dst_len + nchars, "]");
}

static void
acc_to_mem(char * output, int addr, char * src)
{
    // output = "mov [ADDR], ax"
    memcpy(output, "mov [", 5);
    int nchars = sprint_int(output + 5, addr);
    strcpy(output + 5 + nchars, "], ");
    strcpy(output + 8 + nchars, src);
}

// uses hand-rolled sprint
static size_t
dasm_v2(char * bytes, char * output, size_t len)
{
    size_t nbytes = 0;

    int mov_type;
    if      ((bytes[0] & 0xfc) == 0x88) mov_type = MOV_REG_MEM_TO_FROM_REG;
    else if ((bytes[0] & 0xfe) == 0xc6) mov_type = MOV_IMM_TO_REG_MEM;
    else if ((bytes[0] & 0xf0) == 0xb0) mov_type = MOV_IMM_TO_REG;
    else if ((bytes[0] & 0xfe) == 0xa0) mov_type = MOV_MEM_TO_ACC;
    else if ((bytes[0] & 0xfe) == 0xa2) mov_type = MOV_ACC_TO_MEM;
    else if ((bytes[0] & 0xff) == 0x8e) mov_type = MOV_REG_MEM_TO_SEG;
    else if ((bytes[0] & 0xff) == 0x8c) mov_type = MOV_SEG_TO_REG_MEM;
    else assert(0);

    int w;
    int reg;
    int mod = (bytes[1] >> 6) & 0x3;
    int rm  = (bytes[1] >> 0) & 0x7;

    if (mov_type == MOV_IMM_TO_REG) {
        w = (bytes[0] >> 3) & 1;
        reg = bytes[0] & 7;
    } else {
        w = bytes[0] & 1;
        reg = (bytes[1] >> 3) & 0x7;
    }

    // displacement bytes
    uint8_t disp_lo = 0, disp_hi = 0;
    int ndisp = 0;
    if (mov_type == MOV_REG_MEM_TO_FROM_REG ||
        mov_type == MOV_IMM_TO_REG_MEM) {
        if      (mod == 0)  ndisp = (rm == 6) ? 2 : 0;
        else if (mod == 1)  ndisp = 1;
        else if (mod == 2)  ndisp = 2;
        else if (mod == 3)  ndisp = 0;
        if (ndisp == 1) {
            disp_lo = bytes[2];
            disp_hi = (disp_lo & 0x80) ? 0xff : 0;
        } else if (ndisp == 2) {
            disp_lo = bytes[2];
            disp_hi = bytes[3];
        }
    }
    int16_t disp = disp_lo + (disp_hi << 8);

    // data bytes
    int ndata = 0;
    uint8_t data_lo = 0, data_hi = 0;
    if (mov_type == MOV_IMM_TO_REG_MEM || 
        mov_type == MOV_IMM_TO_REG) {
        ndata = w ? 2 : 1;
        int data_offset = 1;
        if (mov_type == MOV_IMM_TO_REG_MEM)
            data_offset = 2 + ndisp;

        if (ndata == 1) {
            data_lo = bytes[data_offset];
        } else if (ndata == 2) {
            data_lo = bytes[data_offset];
            data_hi = bytes[data_offset+1];
        }
    }
    uint16_t data = data_lo + (data_hi << 8);

    // addr bytes
    uint8_t addr_lo = bytes[1];
    uint8_t addr_hi = bytes[2];
    uint16_t addr = addr_lo + (addr_hi << 8);

    nbytes = nbytes_table[mov_type] + ndisp + ndata;

    char disp_str[32] = "";
    char rm_str[32] = "";

    if (mov_type == MOV_REG_MEM_TO_FROM_REG ||
        mov_type == MOV_IMM_TO_REG_MEM) {
        if (mod == 0 && rm == 6) {
            direct_address(rm_str, disp);
        } else if (mod == 3) {
            strcpy(rm_str, reg_decode_table[rm*2 + w]);
        } else {
            displacement(rm_str, rm, disp);
        }
    }

    if (mov_type == MOV_REG_MEM_TO_FROM_REG) {
        char * reg_str = reg_decode_table[reg*2 + w];
        int d = (bytes[0] >> 1) & 0x1;
        char * dst = d ? reg_str : rm_str;
        char * src = d ? rm_str : reg_str;
        reg_mem_to_from_reg(output, dst, src);
    } else if (mov_type == MOV_IMM_TO_REG_MEM) {
        imm_to_reg_mem(output, rm_str, w, data);
    } else if (mov_type == MOV_IMM_TO_REG) {
        char * dst = reg_decode_table[reg*2+w];
        imm_to_reg(output, dst, data);
    } else if (mov_type == MOV_MEM_TO_ACC) {
        char * dst = w ? "ax" : "al";
        mem_to_acc(output, dst, addr);
    } else if (mov_type == MOV_ACC_TO_MEM) {
        char * src = w ? "ax" : "al";
        acc_to_mem(output, addr, src);
    } else {
        fprintf(stderr, "%x\n", bytes[0]);
        assert(0);
    }

    assert(nbytes > 0);
    return nbytes;
}

char file_data[1<<16];

int main(int argc, char * argv[])
{
    if (argc < 2) {
        fprintf(stderr, "usage: %s FILE\n", argv[0]);
        exit(1);
    }

    // guarantee that last 6 bytes are 0
    size_t nread = read_entire_file(argv[1], file_data, sizeof(file_data)-6);

    char dasm_str[32];
    printf("bits 16\n");

    long min_run_time = LONG_MAX;
    size_t nops = 0;
#ifndef NRUNS
#define NRUNS 10000
#endif
    for (int run = 0; run < NRUNS; run++) {
        declare_timer(1);
        start_timer(1);
        size_t i = 0;
        while (i < nread) {
            size_t nbytes = dasm_v2(file_data + i, dasm_str, sizeof(dasm_str));
            i += nbytes;
            if (run == 0) {
                nops++;
                printf("%s\n", dasm_str);
            }
        }
        stop_timer(1);
        long run_time = get_elapsed_ns(1);
        if (run_time < min_run_time)
            min_run_time = run_time;
    }
    //fprintf(stderr, "Input file size: %zu. Number of ops: %zu. Fastest time out of %d: %ldns\n", nread, nops, NRUNS, min_run_time);
    //fprintf(stderr, "%zu\t%ld\n", nread, min_run_time);
    fprintf(stderr, "%ld\n", min_run_time);

    return 0;
}
