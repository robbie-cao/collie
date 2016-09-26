#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "mraa.h"
#include "pn532.h"

static int action(uint8 *data, uint8 dataLen)
{
  // Temp action according to content
  char str[128];

  if (!data) {
    return -1;
  }

  if (data[0] == 0x00) {
    // data[0]     : status code
    // data[1]-[7] : content data from card
    //     [1]     : operation code
    //     [2]-[7] : target board mac address
    switch (data[1]) {
      case 'D':
        // Play a specific voice
        LOG("Play a dedicate void on flash\n");
        system("madplay /root/s/1.mp3 -o wave:- | aplay -D plug:dmix");
        break;
      case 'M':
        // Set communication target
        LOG("Set target talk to\n");
        sprintf(str, "ubus call mua.mqtt.service set_msm_target '{\"DATA\":{\"TO_USR\":\"%02X%02X%02X%02X%02X%02X\"}}'",
            data[2],
            data[3],
            data[4],
            data[5],
            data[6],
            data[7]
            );
        system(str);
        //system("madplay /root/s/2.mp3 -o wave:- | aplay -D plug:dmix");
        {
          uint8_t on_off = 0;
          mraa_gpio_context led_gpio;

          led_gpio = mraa_gpio_init(18);  // 18 -> LED_PIN_STATUS
          on_off = mraa_gpio_read(led_gpio);
          system("ubus call mua.miod.service status_led_blink {}");
          system("ubus call mua.miod.service status_led_off {}");
          mraa_gpio_write(led_gpio, on_off);
        }
        break;
      case 'P':
        // Play a random voice on sd card
        LOG("Play a random void on sdcard\n");
        system("madplay `ls /mnt/mmc/*.mp3 | awk 'BEGIN{ srand(); } { line[NR]=$0 } END{ print line[(int(rand()*NR+1))] }'` -o wave:- | aplay -D plug:dmix");
        break;
      default:
        // Play a random voice on flash
        LOG("Play a random void on flash\n");
        system("madplay `ls /root/r/*.mp3 | awk 'BEGIN{ srand(); } { line[NR]=$0 } END{ print line[(int(rand()*NR+1))] }'` -o wave:- | aplay -D plug:dmix");
        break;
    }
  }
  return 0;
}

int main(void)
{
  uint8 res = 0;
  PN532_FirmwareVersion_t fwVer;

  PN532_Init();

  // LowVbat -> Standby
  PN532_WakeUp();
  sleep_ms(100);
  PN532_WakeUp();       // twice for sure
  sleep_ms(100);
  PN532_SAMConfig(0x01, 0, 0);
#if 0
  PN532_ActiveTarget();
  PN532_InAutoPoll();
#endif
  sleep_ms(100);

  res = PN532_GetFirmwareVersion(&fwVer);
  LOG("PN532_GetFirmwareVersion: 0x%02x\r\n", res);

  while (1) {
#if CHECK_AVAILABILITY
    uint8 found = 0;
    uint8 tgtData[16];
    uint8 len = 0;

    res = PN532_InListPassiveTarget(0x01, 0x00, &found, tgtData, &len);
    LOG("PN532_InListPassiveTarget: 0x%02x\r\n", res);
    if (res != PN532_GOOD) {
      PN532_SendAck();
    }
#else

    uint8 data[32];
    PN532_InListPassiveTarget_Cmd_t cmd = { 0x01, 0x00, {}, 0 };
    PN532_InListPassiveTarget_Resp_t resp = { 0x00, {}, 0 };
    PN532_InListPassiveTarget_Resp_106A_t resp160a;

    res = PN532_InListPassiveTarget2(&cmd, &resp);
    LOG("InLstPasTg: 0x%02x\r\n", res);
    if (res != PN532_GOOD) {
      continue ;
    }
    if (!resp.nbTg) {
      LOG("Not found card!\n");
      continue ;
    }
    res = PN532_InListPassiveTarget_ParseResp(&resp, &resp160a);
    // Check resp UID and managing card
    // TODO
    res = PN532_ReadMifare(&resp160a, data);
    LOG("ReadMifare: 0x%02x\r\n", res);

    res = action(data, 0);

#endif
    sleep(1);
  }
  return 0;
}

/* vim: set ts=2 sw=2 tw=0 list : */
