#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "log.h"
#include "cmd.h"
#include "hex.h"
#include "stm8_cmd.h"

/**
 * Parse cmd string into cmd array
 *
 * input: cmd array, number of cmds
 * output: 0 for success, other for fail
 */
int cmd_parse(const uint8_t *cmd_bytes, unsigned int len, cmd_callback_t cb)
{
    int pos = 0;
    uint16_t cmd_size;
    const uint8_t *p = cmd_bytes;

    if (!cmd_bytes || !len) {
        return -1;
    }

    // Skip bytes until leading sign
    while (p[pos] != 0x55) {
        pos += 1;
    }

    /**
     * byte 0  : 0x55
     * byte 1-2: size
     */
    if (pos >= len - 3) {
        return -1;
    }

    cmd_size = (p[pos + 1] << 8) | p[pos + 2];
    pos += 3;

    if (pos + cmd_size > len) {
        return -1;
    }

    struct stm8_cmd *scmd = (struct stm8_cmd *)(p + pos);
    if (cb) {
        cb(scmd, 1);
    }

    return 0;
}

int cmd_send(const struct stm8_cmd *pcmd, uint8_t cmd_size)
{
    uint8_t i = 0;
    uint8_t buf[32];

    buf[0] = 0x55;
    buf[1] = (cmd_size >> 8) & 0xff;
    buf[2] = cmd_size & 0xff;

    for (i = 0; i < cmd_size; i++) {
        buf[3 + i] = *((uint8_t *)pcmd + i);
    }

    hexdump(buf, cmd_size + 3);

    // Send cmd to UART
    // TODO

    return 0;
}

int led_set(uint8_t led, uint8_t brightness)
{
    uint8_t buf[8];
    struct stm8_cmd *pcmd = (struct stm8_cmd *)buf;

    pcmd->cmd_code = LED_SET;
    pcmd->data[0] = led;
    pcmd->data[1] = brightness;

    cmd_send(pcmd, 3);

    return 0;
}

int vol_set(uint8_t vol)
{
    uint8_t buf[8];
    struct stm8_cmd *pcmd = (struct stm8_cmd *)buf;

    pcmd->cmd_code = VOL_SET;
    pcmd->data[0] = vol;

    cmd_send(pcmd, 2);

    return 0;
}

int vol_get(void)
{
    uint8_t buf[8];
    struct stm8_cmd *pcmd = (struct stm8_cmd *)buf;

    pcmd->cmd_code = VOL_GET;
    pcmd->data[0] = 0;

    cmd_send(pcmd, 2);

    return 0;
}
