#ifndef __CMD_H__
#define __CMD_H__

#include <unistd.h>
#include <stdint.h>

#include "stm8_cmd.h"

/**
 * cmd handler callback
 *
 * input: cmd array, number of cmds
 * output: 0 for success, other for fail
 */
typedef int (*cmd_callback_t)(const struct stm8_cmd *pcmd, uint8_t cmd_size);

int cmd_parse(const uint8_t *cmd_bytes, unsigned int len, cmd_callback_t cb);

int cmd_send(const struct stm8_cmd *pcmd, uint8_t cmd_size);

int led_set(uint8_t led, uint8_t brightness);
int vol_set(uint8_t vol);
int vol_get(void);

#endif
