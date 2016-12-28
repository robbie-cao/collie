#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>   /* File control definitions */
#include <errno.h>   /* Error number definitions */
#include <termios.h> /* POSIX terminal control definitions */

#include <sys/types.h>
#include <sys/time.h>
#include <sys/select.h>
#include <sys/ioctl.h>

#include "log.h"
#include "hex.h"
#include "cmd.h"
#include "uart.h"

#define DEBUG               1

#define TTY_UART            "/dev/ttyS2"

#define BUFFER_SIZE         256

static uint8_t buf_in[BUFFER_SIZE];
#if DEBUG
static uint8_t buf2[BUFFER_SIZE / 2];
#endif


static int cmd_callback(const struct stm8_cmd *pcmd, uint8_t cmd_size)
{
    uint8_t i;
    uint8_t cmd;

    if (!pcmd || !cmd_size) {
        return -1;
    }

    cmd = pcmd->cmd_code;
    printf("CMD: %d\n", pcmd->cmd_code);

    switch (cmd) {
        case LED_GET:
            for (i = 0; i < cmd_size; i += sizeof(struct led_cmd)) {
                printf("LED %d - %d\n", pcmd->data[i], pcmd->data[i + 1]);
            }
            break;
        case BUTTON_GET:
            for (i = 0; i < cmd_size; i += sizeof(struct button_cmd)) {
                printf("BTN %d - %d\n", pcmd->data[i], pcmd->data[i + 1]);
            }
            break;
        case VOL_GET:
            for (i = 0; i < cmd_size; i += sizeof(struct vol_cmd)) {
                printf("VOL %d\n", pcmd->data[i]);
            }
            break;
        case NFC_CARD_INFO:
        case NFC_READ_CARD:
            // TODO
        default:
            break;
    }

    return 0;
}

int main(void)
{
    const char *LOG_TAG = "IOD";
    serial_port sp = NULL;
    int res;

    fprintf(stdout, "Welcome - %s %s\n", __DATE__, __TIME__);

    sp = uart_open(TTY_UART);
    if ((sp == INVALID_SERIAL_PORT) || (sp == CLAIMED_SERIAL_PORT)) {
        perror(TTY_UART);
        exit(-1);
    }
    // We need to flush input to be sure first reply does not comes from older byte transceive
    uart_flush_input(sp, true);
    uart_set_speed(sp, 115200);

    // Test send
    const char *str = "Hello from " TTY_UART "\r\n";
    uart_send(sp, str, strlen(str), 0);

    while (1) {
#if 0
        res = uart_recv(sp, buf_in, 255, NULL, 0);
        buf_in[res] = '\0';

        LOGD(LOG_TAG, "%s\n", buf_in);
        if (strcmp((char *)buf_in, "quit") == 0) {
            break;
        }

#endif
        /**
         * Data format:
         *
         *   1    |    2     |     1     |     n     |
         * --------------------------------------------
         *  0x55  | cmd_size | cmd_code  |  cmd_data |
         *
         * Simulate with ascii, eg: "U03123"
         */
        int len = 0;

        res = uart_receive(sp, buf_in, 1, NULL, 0);
        LOGD(LOG_TAG, "Heading: %c\n", buf_in[0]);

        if (buf_in[0] == 'U') {
            res = uart_receive(sp, buf_in, 2, NULL, 0);
            len = (hex2digit(buf_in[0]) << 8) | hex2digit(buf_in[1]);
            LOGD(LOG_TAG, "Size: %d\n", len);
        }

        if (len) {
            res = uart_receive(sp, buf_in, len, NULL, 0);
            hexdump(buf_in, len);
        }

        printf("...\n");
    }

    D();
    uart_close(sp);

    return 0;
}

/* vim: set ts=4 sw=4 tw=0 list : */
