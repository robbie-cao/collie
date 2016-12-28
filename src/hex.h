#ifndef __HEX_H__
#define __HEX_H__

#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* Convert a hex char digit to its integer value. */
int hex2digit(char digit);

/* Decode a hex string. */
int hex2data(unsigned char *data, const char *hexstring, unsigned int len);

int data2hex(char *hexstring, const unsigned char *data, unsigned int len);

void hexdump(const unsigned char *data, unsigned int len);

#endif
