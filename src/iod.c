#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>   /* File control definitions */
#include <errno.h>   /* Error number definitions */
#include <termios.h> /* POSIX terminal control definitions */
#include <pthread.h>

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

pthread_t thread_stdin, thread_ttysin;
serial_port sp = NULL;

static int cmd_handler(const struct stm8_cmd *pcmd, uint8_t cmd_size)
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

void thread_stdin_handler(int *arg)
{
    const char *LOG_TAG = "STDIN";
    char input[128];
    fd_set readfds;

    (void) arg;     // Make compiler happy

    fprintf(stdout, "-> STDIN Input\n");

    FD_ZERO(&readfds);
    FD_SET(STDIN_FILENO, &readfds);

    while (1) {
        if (select(1, &readfds, NULL, NULL, NULL)) {
            int ret = scanf("%s", input);

            if (ret) {
                LOGD(LOG_TAG, "%s\n", input);
            }

            switch (input[0]) {
                case '1':
                    printf("Simulate setting led\n");
                    uart_send(sp, input, strlen(input), 0);
                    break;
                case '2':
                    printf("Simulate read/write NFC\n");
                    uart_send(sp, input, strlen(input), 0);
                    break;
                default:
                    uart_send(sp, input, strlen(input), 0);
                    break;
            }
        }

        printf("...\n");
    }
    fprintf(stdout, "<- STDIN Input\n");
}

void thread_ttys_handler(int *arg)
{
    const char *LOG_TAG = "TTYS2";
    char input[BUFFER_SIZE];
    fd_set readfds;
    int res;

    (void) arg;     // Make compiler happy

    fprintf(stdout, "-> TTYS2 Input\n");

    while (1) {
#if 1
        // Test uart_recv
        res = uart_recv(sp, input, BUFFER_SIZE - 1, NULL, 0);
        input[res] = '\0';

        LOGD(LOG_TAG, "%s\n", input);
        if (strcmp((char *)input, "quit") == 0) {
            break;
        }

#else

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

        res = uart_receive(sp, input, 1, NULL, 0);
        LOGD(LOG_TAG, "Heading: 0x%02x\n", input[0]);

        if (input[0] == 0x55) {    // 0x55 => 'U'
            res = uart_receive(sp, input, 2, NULL, 0);
#if 0 // ASCII
            len = (hex2digit(input[0]) << 8) | hex2digit(input[1]);
#else
            len = (input[0] << 8) | input[1];
#endif
            LOGD(LOG_TAG, "Size: 0x%02x %02x - %d\n", input[0], input[1], len);
        }

        if (len) {
            res = uart_receive(sp, input, len, NULL, 0);
            hexdump(input, len);
        }
#endif

        printf("...\n");
    }

    fprintf(stdout, "<- TTYS2 Input\n");
}

int main(void)
{
    const char *LOG_TAG = "IOD";

    fprintf(stdout, "Welcome - %s %s\n", __DATE__, __TIME__);

    /* Init serial port */
    sp = uart_open(TTY_UART);
    if ((sp == INVALID_SERIAL_PORT) || (sp == CLAIMED_SERIAL_PORT)) {
        perror(TTY_UART);
        exit(-1);
    }
    // We need to flush input to be sure first reply does not comes from older byte transceive
    uart_flush_input(sp, true);
    uart_set_speed(sp, 115200);

    pthread_create(&thread_stdin,
            NULL,
            (void *) thread_stdin_handler,
            (void *) NULL);
    pthread_create(&thread_ttysin,
            NULL,
            (void *) thread_ttys_handler,
            (void *) NULL);

    pthread_join(thread_stdin, NULL);
    pthread_join(thread_ttysin, NULL);

    /* Clean up */
    uart_close(sp);

    pthread_exit(NULL);

    return 0;
}

/* vim: set ts=4 sw=4 tw=0 list : */
