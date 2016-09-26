#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "mraa.h"
#include "pn532.h"

void usage(char *name)
{
  printf("Usage:\n");
  printf("\t%s read\n", name);
  printf("\t%s write opcode data...\n", name);
}

int main(int argc, char **argv)
{
  uint8 res = 0;
  PN532_FirmwareVersion_t fwVer;
  uint8 rw = 0;     // 0 - read, 1 - write, others - not support
  uint8 data[32];

  if (argc < 2) {
    usage(argv[0]);

    return 1;
  }

  if (!strcmp(argv[1], "read")) {
    rw = 0;
  } else if (!strcmp(argv[1], "write")) {
    rw = 1;
  } else {
    printf("Not support!\n");
    usage(argv[0]);

    return 1;
  }

  if (rw && argc < 4) {
    printf("Invalid parameters!\n");
    usage(argv[0]);

    return 1;
  }

  if (rw) {
    uint8 opcode = argv[2][0];
    uint64_t macaddr = strtoll(argv[3], NULL, 16);

    memset(data, 0, sizeof(data));

    data[0] = opcode;
    data[1] = (macaddr >> (8 * 5)) & 0xFF;
    data[2] = (macaddr >> (8 * 4)) & 0xFF;
    data[3] = (macaddr >> (8 * 3)) & 0xFF;
    data[4] = (macaddr >> (8 * 2)) & 0xFF;
    data[5] = (macaddr >> (8 * 1)) & 0xFF;
    data[6] = (macaddr >> (8 * 0)) & 0xFF;
  }

  PN532_Init();

  // LowVbat -> Standby
  PN532_WakeUp();
  sleep_ms(100);
  PN532_WakeUp();       // twice for sure
  sleep_ms(100);
  PN532_SAMConfig(0x01, 0, 0);
  sleep_ms(100);

  res = PN532_GetFirmwareVersion(&fwVer);
  LOG("PN532_GetFirmwareVersion: 0x%02x\r\n", res);

  while (1) {
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
    if (rw) {
      // Write
      res = PN532_WriteMifare(&resp160a, data, 16);
      LOG("WriteMifare: 0x%02x\r\n", res);
    } else {
      // Read
      res = PN532_ReadMifare(&resp160a, data);
      LOG("ReadMifare: 0x%02x\r\n", res);
    }

    sleep(1);

    if (res == PN532_GOOD) {
      return 0;
    }
  }
  return 0;
}

/* vim: set ts=2 sw=2 tw=0 list : */
