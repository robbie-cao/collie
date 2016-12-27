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

#include "log.h"

#define TTY_UART    "/dev/ttyS2"

#define BUFFER_SIZE         256

static uint8_t buf_in[BUFFER_SIZE];
#if DEBUG
static uint8_t buf2[BUFFER_SIZE / 2];
#endif

int main(void)
{
    const char *LOG_TAG = "IOD";
    int            fd;
    struct termios options, old;
    fd_set         readfds;

    int idx = 0;

    fprintf(stdout, "Welcome - %s %s\n", __DATE__, __TIME__);

    /* Open the port */
    fd = open(TTY_UART, O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd < 0) {
        perror(TTY_UART);
        exit(-1);
    }
    fcntl(fd, F_SETFL, 0);

#if 0
    // Test write
    while (1) {
        int n = write(fd, "ATZ\r\n", 5);
        if (n < 0) {
            fputs("write() of 4 bytes failed!\n", stderr);
        } else {
            fputs("write() of 4 bytes success!\n", stderr);
        }
        sleep(1);
    }

    // Test read
    while (1) {     /* loop until we have a terminating condition */
        /* read blocks program execution until a line terminating character is
         * input, even if more than 255 chars are input. If the number
         * of characters read is smaller than the number of chars available,
         * subsequent reads will return the remaining chars. res will be set
         * to the actual number of characters actually read
         */
        uint8_t buf[255];
        int res = read(fd, buf, 255);

        buf[res] = 0;             /* set end of string, so we can printf */
        printf(":%s:%d\n", buf, res);
    }
#endif

#if 0
    /*
     * Get the current options for the port...
     */
    tcgetattr(fd, &old);

    bzero(&options, sizeof(options));

    /*
     * BAUDRATE: Set bps rate. You could also use cfsetispeed and cfsetospeed.
     * CRTSCTS : output hardware flow control (only used if the cable has
     * all necessary lines. See sect. 7 of Serial-HOWTO)
     * CS8     : 8n1 (8bit,no parity,1 stopbit)
     * CLOCAL  : local connection, no modem contol
     * CREAD   : enable receiving characters
     */
    options.c_cflag = B115200 /* | CRTSCTS */ | CS8 | CLOCAL | CREAD;

    /*
     * Clean the modem line
     */
    tcflush(fd, TCIFLUSH);

    /*
     * Set the new options for the port...
     */
    tcsetattr(fd, TCSANOW, &options);
#endif
    tcgetattr(fd, &options);
    options.c_cc[VMIN]  = 0;
    options.c_cc[VTIME] = 10;
    tcsetattr(fd, TCSANOW, &options);

    FD_ZERO(&readfds);
    FD_SET(STDIN_FILENO, &readfds); /* stdin */
    FD_SET(fd, &readfds);           /* ttyS2 */

    while (1) {
        // Wait for input from stdin
        if (select(fd + 1, &readfds, NULL, NULL, NULL)) {
            int c;

            printf("Wakeup\n");
            idx = 0;

            if (FD_ISSET(STDIN_FILENO, &readfds)) {
                printf("fd 0 is set\n");
                // Store input into buf_in, newline ('\n') as input done
                while ((c = fgetc(stdin)) != '\n') {
                    buf_in[idx++] = c;
                }
                buf_in[idx] = '\0';
            }
            if (FD_ISSET(fd, &readfds)) {
                printf("fd %d is set\n", fd);
                // Store input into buf_in, newline ('\n') as input done
                int res = read(fd, buf_in, 255);
                buf_in[res] = '\0';
            }

            LOGD(LOG_TAG, "%s\n", buf_in);
        }

        if (strcmp((char *)buf_in, "quit") == 0) {
            break;
        }

        printf("...\n");

        // Parse buffer and action accordingly
#if DEBUG
        // Test send cmd
        if (strcmp((char *)buf_in, "send") == 0) {
            led_set(1, 20);
            led_set(2, 0);
            vol_get();
            vol_set(50);

            continue;
        }

        // Test receive cmd
        int len = hex2data(buf2, (char *)buf_in, strlen((char *)buf_in));
        hexdump(buf2, len);
        cmd_parse(buf2, len, cmd_callback);
#endif
    }

    close(fd);

    return 0;
}

/* vim: set ts=4 sw=4 tw=0 list : */
