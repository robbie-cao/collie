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

#define DEBUG               1

#define TTY_UART            "/dev/ttyS2"

#define BUFFER_SIZE         256

static uint8_t buf_in[BUFFER_SIZE];
#if DEBUG
static uint8_t buf2[BUFFER_SIZE / 2];
#endif

void sanity(void)
{
    int            fd;

    /* Open the port */
    fd = open(TTY_UART, O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd < 0) {
        perror(TTY_UART);
        exit(-1);
    }
    fcntl(fd, F_SETFL, 0);

    // Test write
    if (1) {
        int n = write(fd, "ATZ\r\n", 5);
        if (n < 0) {
            fputs("write() of 5 bytes failed!\n", stderr);
        } else {
            fputs("write() of 5 bytes success!\n", stderr);
        }
        sleep(1);
    }

    // Test read
    if (1) {     /* loop until we have a terminating condition */
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

    close(fd);
}

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

int main(void)
{
    const char *LOG_TAG = "IOD";
    int            fd;
    struct termios options, old;
    fd_set         readfds;

    fprintf(stdout, "Welcome - %s %s\n", __DATE__, __TIME__);

    /*
     * Init
     */
    // Open the port
    fd = open(TTY_UART, O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd < 0) {
        perror(TTY_UART);
        exit(-1);
    }
    fcntl(fd, F_SETFL, 0);

    // Get the current options for the port...
    tcgetattr(fd, &old);
    options = old;

    // BAUDRATE: Set bps rate. You could also use cfsetispeed and cfsetospeed.
    // CS8     : 8n1 (8bit, no parity, 1 stopbit)
    // CLOCAL  : local connection, no modem contol
    // CREAD   : enable receiving characters
    options.c_cflag = B115200 | CS8 | CLOCAL | CREAD;
    options.c_iflag = IGNPAR;
    options.c_oflag = 0;
    options.c_lflag = 0;

    options.c_cc[VMIN]  = 0;        // block until n bytes are received
    options.c_cc[VTIME] = 0;        // block until a timer expires (n * 100 ms)

    // Clean the modem line
    tcflush(fd, TCIFLUSH);

    // Set the new options for the port...
    tcsetattr(fd, TCSANOW, &options);


    FD_ZERO(&readfds);
    FD_SET(fd, &readfds);           /* ttyS2 */

    while (1) {
        int res;

        // Wait for input from ttyS2
        res = select(fd + 1, &readfds, NULL, NULL, NULL);

        if ((res < 0) && (EINTR == errno)) {
            // The system call was interupted by a signal and a signal handler was
            // run.  Restart the interupted system call.
            fprintf(stderr, "errro on select!\n");
        }

        if (FD_ISSET(fd, &readfds)) {
            int nr;
            int available;

            // Retrieve the count of the incoming bytes
            res = ioctl(fd, FIONREAD, &available);

            // Store input into buf_in, newline ('\n') as input done
            nr = read(fd, buf_in, available);
            buf_in[nr] = '\0';
        }

        LOGD(LOG_TAG, "%s\n", buf_in);

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
        cmd_parse(buf2, len, cmd_handler);
#endif
    }

    // Restore port config
    tcsetattr(fd, TCSANOW, &old);

    close(fd);

    return 0;
}

/* vim: set ts=4 sw=4 tw=0 list : */
