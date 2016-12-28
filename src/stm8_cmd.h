#ifndef __STM8_CMD_H__
#define __STM8_CMD_H__

#ifdef STM8
#include "stm8l15x.h"
#endif

/**
 * header   : 0x55
 * cmd_size : from cmd_code to end
 * cmd_code :
 *
 *   1    |    2     |     1     |     n     |
 * --------------------------------------------
 *  0x55  | cmd_size | cmd_code  |  cmd_data |
 */

#define LED_SET             1
#define LED_GET             2

#define BUTTON_SET          10
#define BUTTON_GET          11

#define NFC_CARD_INFO       20
#define NFC_WRITE_CARD      21
#define NFC_READ_CARD       21

#define VOL_SET             40
#define VOL_GET             41

struct stm8_cmd {
  uint8_t cmd_code;
  uint8_t data[0];     //cmd_data,
};

//define how command transfer
struct stm8_tran {
  uint8_t start_code;
  uint16_t cmd_size;
  struct stm8_cmd cmd;
};

typedef struct {
  uint16_t cmd_size;
  struct stm8_cmd cmd;
} CMD_T;

//led cmd_data struct,can transfer zero or more once
enum LED_NUM {
  LED1 = 0,
  LED2,
  LED3,
  LED4,
  LED5,
  LED6,
  LED7,
  LED8
};

struct led_cmd {
  uint8_t led_num;   //led number
  uint8_t led_brightness; //0:led off    1~99:led brightness from 1% to 99%  100:led on
};

//button cmd_data strutc,can transfer zero or more once
enum BUTTON_STATUS {
  BUTTON_PRESS = 0,
  BUTTON_RELEASE ,

  BUTTON_PRESS_1S = 10,
  BUTTON_PRESS_2S,
  BUTTON_PRESS_3S,
  BUTTON_PRESS_5S,
  BUTTON_PRESS_10S,
  BUTTON_PRESS_15S,
  BUTTON_PRESS_30S,
};

enum BUTTON_CODE {
  SW1 = 0,
  SW2,
  SW3,
  SW4,
  SW5,
  SW6,
  SW7,
  SW8,
  SW9,
  SW10,
  SW11,
  SW12,
  SW13,
  SW14,
  SW15
};

struct button_cmd {
  uint8_t button_code;  //button number
  enum BUTTON_STATUS button_status;  //button_status
};

//vol cmd_data strutc,can transfer one or zero
struct vol_cmd {
  uint8_t vol; //0~100:vol 0 to 100%
};

//nfc cmd data,define letter

#endif
