#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
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

static size_t
dasm(char * bytes, char * buffer, size_t len)
{
    size_t nbytes = 0;

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
            int nprint = snprintf(buffer, len, "mov %s, %s", dst, src);
            assert(nprint < len);
        } else if (mod == 1) {
            // 1 byte disp
            uint8_t disp_lo = bytes[2];
            snprintf(disp_str, sizeof(disp_str), " + %" PRId8, disp_lo);
            snprintf(rm_str, sizeof(rm_str), rm_str_fmt, disp_str);
            int nprint = snprintf(buffer, len, "mov %s, %s", dst, src);
            assert(nprint < len);
            nbytes = 3;
        } else if (mod == 2) {
            // 2 byte disp
            uint8_t disp_lo = bytes[2];
            uint8_t disp_hi = bytes[3];
            uint16_t disp = disp_lo + (disp_hi << 8);
            snprintf(disp_str, sizeof(disp_str), " + %" PRId16, disp);
            snprintf(rm_str, sizeof(rm_str), rm_str_fmt, disp_str);
            int nprint = snprintf(buffer, len, "mov %s, %s", dst, src);
            assert(nprint < len);
            nbytes = 4;
        } else if (mod == 3) {
            char * rm_str  = reg_decode_table[rm*2 + w];
            dst = d ? reg_str : rm_str;
            src = d ? rm_str : reg_str;
            int nprint = snprintf(buffer, len, "mov %s, %s", dst, src);
            assert(nprint < len);
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
            // no displacement
            assert(rm != 6);
            data_lo = bytes[2];
            data_hi = w ? bytes[3] : 0;
            nbytes = w ? 4 : 3;
        } else if (mod == 1) {
            // 1 byte disp
            disp_lo = bytes[2];
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
        snprintf(disp_str, sizeof(disp_str), " + %" PRId16, disp);
        snprintf(rm_str, sizeof(rm_str), rm_str_fmt, disp_str);
        char * size = w ? "word" : "byte";
        int nprint = snprintf(buffer, len, "mov %s, %s %" PRIu16, rm_str, size, data);
        assert(nprint < len);
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
        int nprint = snprintf(buffer, len, "mov %s, %s %" PRIu16, dst, size, imm);
        assert(nprint < len);
    } else if ((bytes[0] & 0xfe) == 0xa0) {
        // mem to ax
        int w = bytes[0] & 1;
        uint8_t addr_lo = bytes[1];
        uint8_t addr_hi = bytes[2];
        uint16_t addr = addr_lo + (addr_hi << 8);
        char * dst = w ? "ax" : "al";
        nbytes = 3;
        int nprint = snprintf(buffer, len, "mov %s, [%" PRIu16 "]", dst, addr);
        assert(nprint < len);
    } else if ((bytes[0] & 0xfe) == 0xa2) {
        // ax to mem
        int w = bytes[0] & 1;
        uint8_t addr_lo = bytes[1];
        uint8_t addr_hi = bytes[2];
        uint16_t addr = addr_lo + (addr_hi << 8);
        char * src = w ? "ax" : "al";
        nbytes = 3;
        int nprint = snprintf(buffer, len, "mov [%" PRIu16 "], %s", addr, src);
        assert(nprint < len);
    } else if ((bytes[0] & 0xff) == 0x8e) {
        assert(0);
    } else if ((bytes[0] & 0xff) == 0x8c) {
        assert(0);
    } else {
        fprintf(stderr, "%x\n", bytes[0]);
        assert(0);
    }

    assert(nbytes > 0);
    return nbytes;
}

int main(int argc, char * argv[])
{
    if (argc < 2) {
        fprintf(stderr, "usage: %s FILE\n", argv[0]);
        exit(1);
    }

    char data[4096];
    size_t nread = read_entire_file(argv[1], data, sizeof(data));

    char dasm_str[32];
    printf("bits 16\n");
    size_t i = 0;
    while (i < nread) {
        size_t nbytes = dasm(data + i, dasm_str, sizeof(dasm_str));
        i += nbytes;
        printf("%s\n", dasm_str);
    }

    return 0;
}
