#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "pn532.h"

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
    PN532_Test();
#endif
    sleep(1);
  }
  return 0;
}

/* vim: set ts=2 sw=2 tw=0 list : */
