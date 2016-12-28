#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "hex.h"

/* Convert a hex char digit to its integer value. */
int hex2digit(char digit)
{
    // 0-9
    if ('0' <= digit && digit <= '9') {
        return (int)(digit - '0');
    }
    // a-f
    if ('a' <= digit && digit <= 'f') {
        return (int)(10 + digit - 'a');
    }
    // A-F
    if ('A' <= digit && digit <= 'F') {
        return (int)(10 + digit - 'A');
    }

    return 0;
}

/* Decode a hex string. */
int hex2data(unsigned char *data, const char *hexstring, unsigned int len)
{
    size_t count = 0;

    if (!hexstring || !data || !len) {
        return 0;
    }

    if (strlen(hexstring) == 0) {
        return 0;
    }

    for (count = 0; count < (len / 2); count++) {
        data[count] = hex2digit(hexstring[count * 2]);
        data[count] <<= 4;
        data[count] |= hex2digit(hexstring[count * 2 + 1]);
    }

    return count;
}

int data2hex(char *hexstring, const unsigned char *data, unsigned int len)
{
    size_t count = 0;

    if (!hexstring || !data || !len) {
        return 0;
    }

    for (count = 0; count < len; count++) {
        sprintf(hexstring, "%02x", data[count]);
    }
    hexstring[count * 2] = '\0';

    return strlen(hexstring);
}

void hexdump(const unsigned char *data, unsigned int len)
{
    int i = 0;

    for (i = 0; i < len; i++) {
        printf("%02x ", data[i]);
    }
    printf("\n");
}
