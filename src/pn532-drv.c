#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "mraa.h"
#include "pn532.h"

#define LOG_TAG             "PN5"

#define DEBUG_PN532         1
#define DEBUG_PN532_BYTE    1
#define DEBUG_PN532_PACKET  1

#define PN532_DATABUF_MAX   32

#define PN532_ACK_PACKET_LEN   6

#define LOG_LEVEL_SYMBOL_VERBOSE        "V"
#define LOG_LEVEL_SYMBOL_INFO           "I"
#define LOG_LEVEL_SYMBOL_DEBUG          "D"
#define LOG_LEVEL_SYMBOL_WARN           "W"
#define LOG_LEVEL_SYMBOL_ERROR          "E"

#define LOG(fmt, arg...)                printf(fmt, ##arg)
#define LOGV(tag, fmt, arg...)          printf(LOG_LEVEL_SYMBOL_VERBOSE ## "\t" ## tag ## "\t" fmt, ## arg)
#define LOGI(tag, fmt, arg...)          printf(LOG_LEVEL_SYMBOL_INFO    ## "\t" ## tag ## "\t" fmt, ## arg)
#define LOGD(tag, fmt, arg...)          printf(LOG_LEVEL_SYMBOL_DEBUG   ## "\t" ## tag ## "\t" fmt, ## arg)
#define LOGW(tag, fmt, arg...)          printf(LOG_LEVEL_SYMBOL_WARN    ## "\t" ## tag ## "\t" fmt, ## arg)
#define LOGE(tag, fmt, arg...)          printf(LOG_LEVEL_SYMBOL_ERROR   ## "\t" ## tag ## "\t" fmt, ## arg)

#define D(fmt, arg...)                  printf("Line %d:\t" fmt, __LINE__, ##arg)
#define DD()                            printf("Line %d, %s\n", __LINE__, __FUNCTION__)

#define UART_PORT           1
#define UART_BAUDRATE       115200

static mraa_uart_context uart;

static uint8 sDataBuf[PN532_DATABUF_MAX];
static uint8 sWaitAck = 0;          // Flag set after sending cmd, send ACK on the coming IRQ from PN532

static uint8 PN532_ACK_FRAME[] = { 0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00 };

static uint8 resp_buf[256];

static sleep_ms(int ms) {
  struct timeval tv;

  tv.tv_sec = ms / 1000;
  tv.tv_usec = (ms % 1000) * 1000;
  select(0, NULL, NULL, NULL, &tv);
}

static void dump(uint8 *buf, int len)
{
  uint8 i;

  for (i = 0; i < len; i++) {
    LOG("%02x ", *(buf + i));
  }
  LOG("\r\n");
}

uint8 PN532_Write(uint8 *data, uint8 len)
{
  uint8 res = PN532_BUS_BUSY;
  uint8 i = 0;

  if (!data || !len) {
    return PN532_INVALID_PARAM;
  }

  res = mraa_uart_write(uart, data, len);

#if DEBUG_PN532_BYTE
  LOG("W - %02d - 0x", res);
  for (i = 0; i < len; i++) {
    LOG("%02x ", *(data + i));
  }
  LOG("\r\n");
#endif


  return (res == MRAA_SUCCESS ? PN532_GOOD : PN532_BUS_BUSY);
}

uint8 PN532_Read(uint8 *data, uint8 len)
{
  uint8 res = PN532_BUS_BUSY;
  uint8 i = 0;

  if (!data || !len) {
    return PN532_INVALID_PARAM;
  }

  res = mraa_uart_read(uart, data, len);

#if DEBUG_PN532_BYTE
  LOG("R - %02d - 0x", res);
  for (i = 0; i < res; i++) {
    LOG("%02x ", *(data + i));
  }
  LOG("\r\n");
#endif

  return (res == len) ? PN532_GOOD : PN532_BUS_BUSY;
}

/**
 * +-----+-----+-----------+
 * | TFI | PD0 | PD1...PDn |
 * +-----+-----+-----------+
 * | D4  | cmd | cmd data  |
 * +-----+-----+-----------+
 */
uint8 PN532_SendCmd(uint8 cmd, uint8 *pCmdData, uint8 dataLen, uint8 waitAck)
{
  uint8 res = PN532_BUS_BUSY;
  uint8 idx = 0;
  uint8 sum = 0;
  uint8 i = 0;

  if (!pCmdData && dataLen) {
    return PN532_INVALID_PARAM;
  }

  sDataBuf[idx++] = PN532_PREAMBLE;
  sDataBuf[idx++] = PN532_STARTCODE_0;
  sDataBuf[idx++] = PN532_STARTCODE_1;
  sDataBuf[idx++] = dataLen + 2;         // Length of (TFI, PD0...PDn)
  sDataBuf[idx++] = ~(dataLen + 2) + 1;  // Length Checksum
  sDataBuf[idx++] = PN532_TFI_HOST2PN;   // Frame Identifier
  sDataBuf[idx++] = cmd;                 // PD0

  sum = PN532_TFI_HOST2PN + cmd;
  // PD1...PDn (Optional Input Data)
  for (i = 0; i < dataLen; i++) {
    sDataBuf[idx++] = pCmdData[i];
    sum += pCmdData[i];
  }
  sDataBuf[idx++] = (~sum) + 1;          // Data Checksum
  sDataBuf[idx++] = PN532_POSTAMBLE;

  res = mraa_uart_write(uart, sDataBuf, idx);
#if DEBUG_PN532_BYTE
  LOG("W - %02d - 0x", res);
  for (i = 0; i < idx; i++) {
    LOG("%02x ", *(sDataBuf + i));
  }
  LOG("\r\n");
#endif

  sWaitAck = (res == idx ? waitAck : 0);

  return (res == idx ? PN532_GOOD : PN532_BUS_BUSY);
}

uint8 PN532_SendAck(void)
{
  uint8 res = PN532_BUS_BUSY;
  uint8 len = sizeof(PN532_ACK_FRAME);

  res = mraa_uart_write(uart, PN532_ACK_FRAME, len);

  return (res == len ? PN532_GOOD : PN532_BUS_BUSY);
}

/**
 * Return
 * 0      - ACK
 * 1      - NACK
 * Others - ERROR
 */
int8 PN532_ReadAck(void)
{
  uint8 res = PN532_BUS_BUSY;
  uint8 i = 0;

  res = mraa_uart_read(uart, sDataBuf, PN532_ACK_PACKET_LEN);

#if DEBUG_PN532_BYTE
  LOG("R - %02d - 0x", res);
  for (i = 0; i < res; i++) {
    LOG("%02x ", *(sDataBuf + i));
  }
  LOG("\r\n");
#endif

  res = PN532_FrameParser(sDataBuf, res, NULL, NULL);

  LOG("ReadAck: 0x%02x - %s\r\n", res, !res ? "ACK" : "NACK");
  if (res == PN532_TFI_ACK) {
    return PN532_ACK;
  }
  if (res == PN532_TFI_NACK) {
    return PN532_NACK;
  }

  return res;
}

/**
 * Return the number of bytes successfully read.
 */
int8 PN532_ReadRsp(uint8 *pResp)
{
  int8 res = PN532_BUS_BUSY;
  uint8 i = 0;

  if (!pResp) {
    return PN532_INVALID_PARAM;
  }

  res = mraa_uart_read(uart, pResp, PN532_DATABUF_MAX);

  LOG("ReadRsp: 0x%02x\r\n", res);
#if DEBUG_PN532_BYTE
  LOG("R - %02d - 0x", res);
  for (i = 0; i < res; i++) {
    LOG("%02x ", *(pResp + i));
  }
  LOG("\r\n");
#endif

  return res;
}

/**
 * PN532 generates IRQ signal to inform that the answer is ready
 * (both ACK frame and answer response frame).
 * \code{.c}
 * IRQ -----------------+  +------------------------+        +--------------------------------
 *                      |  |                        |        |
 *                      +--+                        +--------+
 * SDA -------+-------+---+------------+--------------------+-------------+-------------------
 *            |  CMD  |   | Read + ACK |                    | Read + RESP |
 *            +-------+   +------------+                    +-------------+
 * SCL -------+-------+---+------------+--------------------+-------------+-------------------
 *            |  CLK  |   |     CLK    |                    |     CLK     |
 *            +-------+   +------------+                    +-------------+
 * \endcode
 *
 * However, if the host controller starts an exchange asserting the H_REQ signal,
 * PN532 will react as described below.
 * \code{.c}
 * REQ ---+ +---------------------------------------------------------------------------------
 *        | |
 *        +-+
 * IRQ ------+ +--------+  +------------------------+        +--------------------------------
 *           | |        |  |                        |        |
 *           +-+        +--+                        +--------+
 * SDA -------+-------+---+------------+--------------------+-------------+-------------------
 *            |  CMD  |   | Read + ACK |                    | Read + RESP |
 *            +-------+   +------------+                    +-------------+
 * SCL -------+-------+---+------------+--------------------+-------------+-------------------
 *            |  CLK  |   |     CLK    |                    |     CLK     |
 *            +-------+   +------------+                    +-------------+
 * \endcode
 */

/**
 * Input:
 * +----+----+
 * | D4 | 02 |
 * +----+----+
 * Output:
 * +----+----+----+-----+-----+---------+
 * | D5 | 03 | IC | Ver | Rev | Support |
 * +----+----+----+-----+-----+---------+
 */
uint8 PN532_GetFirmwareVersion(PN532_FirmwareVersion_t *pVer)
{
  int8 res = PN532_BUS_BUSY;
  uint8 tfi = 0;
  uint8 len = 0;
  uint8 *pPacket = NULL;

  LOG("## GetFirmwareVersion\r\n");

  LOG("## SendCmd\r\n");
  res = PN532_SendCmd(PN532_CMD_GETFIRMWAREVERSION, NULL, 0, 0);
  if (res != PN532_GOOD) {
    return res;
  }
  sleep_ms(100);

  LOG("## ReadAck\r\n");
  if (PN532_ReadAck() != PN532_ACK) {
    return PN532_INVALID_ACK;
  }
  sleep_ms(100);

  LOG("## ReadRsp\r\n");
  memset(sDataBuf, 0, sizeof(sDataBuf));    // Clean buffer for sure
  res = PN532_ReadRsp(sDataBuf);
  if (res <= 0) {
    return PN532_ERR;
  }

  LOG("## Decode RspFrame\r\n");
  // Decode response frame
  tfi = PN532_FrameParser(sDataBuf, res, (void **)&pPacket, &len);
  if (tfi != PN532_TFI_PN2HOST) {
    return PN532_INVALID_FRAME;
  }
  if (!pPacket
      || pPacket[0] != PN532_CMD_GETFIRMWAREVERSION + 1
      || len != sizeof(PN532_FirmwareVersion_t) + 1
     ) {
    return PN532_INVALID_FRAME;
  }

  if (!pVer) {
    return PN532_INVALID_PARAM;
  }

  pVer->ic      = pPacket[1];
  pVer->ver     = pPacket[2];
  pVer->rev     = pPacket[3];
  pVer->support = pPacket[4];

#if DEBUG_PN532_PACKET
  LOG("IC : %02x\r\n", pPacket[1]);
  LOG("VER: %02x\r\n", pPacket[2]);
  LOG("REV: %02x\r\n", pPacket[3]);
  LOG("SUP: %02x\r\n", pPacket[4]);
#endif

  return PN532_GOOD;
}

/* Input
 * +----+----+-------+------+------------+
 * | D4 | 4A | MaxTg | BrTy | [InitData] |
 * +----+----+-------+------+------------+
 * Output
 * +----+----+------+----------------+----------------+
 * | D5 | 4B | NbTg | [TgtData1 [] ] | [TgtData2 [] ] |
 * +----+----+------+----------------+----------------+
 * Return
 * Success or Error
 */
uint8 PN532_InListPassiveTarget(PN532_InListPassiveTarget_Cmd_t *pCmd, PN532_InListPassiveTarget_Resp_t *pResp)
{
  int8 res = PN532_BUS_BUSY;
  uint8 tfi = 0;
  uint8 len = 0;
  uint8 *pPacket = NULL;
  uint8 cmdData[] = { 0x01, 0x00 };     // MaxTg, BrTy
  uint8 i = 0;

  // Input pCmd -> cmdData
  // TODO
#if 0
  if (!pCmd) {
    return PN532_INVALID_PARAM;
  }
#endif

  res = PN532_SendCmd(PN532_CMD_INLISTPASSIVETARGET, cmdData, sizeof(cmdData), 0);
  if (res != PN532_GOOD) {
    return res;
  }

  if (PN532_ReadAck() != PN532_ACK) {
    return PN532_INVALID_ACK;
  }

  sleep_ms(50);                             // FIXME: necessary for POLL? No need for IRQ

  memset(sDataBuf, 0, sizeof(sDataBuf));    // Clean buffer for sure
  res = PN532_ReadRsp(sDataBuf);
  if (res <= 0) {
    return PN532_ERR;
  }

  // Decode response frame
  tfi = PN532_FrameParser(sDataBuf, res, (void **)&pPacket, &len);
  if (tfi != PN532_TFI_PN2HOST) {
    return PN532_INVALID_FRAME;
  }
  if (!pPacket
      || pPacket[0] != PN532_CMD_INLISTPASSIVETARGET + 1
     ) {
    return PN532_INVALID_FRAME;
  }

#if 0
  // TODO
  // Copy response data to output
  if (!pResp) {
    return PN532_INVALID_PARAM;
  }
#endif

#if DEBUG_PN532_PACKET
  LOG("I - %02d - 0x", len);
  for (i = 0; i < len; i++) {
    LOG("%02x ", *(pPacket + i));
  }
  LOG("\r\n");
#endif

  return PN532_GOOD;
}

uint8 PN532_RespHandler(void)
{
  // TODO
  return 0;
}

/**
 * Input
 * - Frame bytes
 * Output
 * - Pointer to Packet Data (position in input frame)
 * - Packet Data Length (Not include TFI)
 * Return
 * - TFI (frame identifier) for valid frame (0 for unknown)
 * - Other for invlid frame or error
 */
uint8 PN532_FrameParser(const uint8 *pFrame, uint8 frmLen, void **ppPacket, uint8 *pLen)
{
  uint8 len = 0;
  uint8 sum = 0;
  uint8 idx = 0;
  uint8 checksum = 0;
  const uint8 *p = pFrame;
  uint8 i = 0;

  if (!pFrame || frmLen < PN532_FRAME_LEN_MIN) {
    return PN532_INVALID_FRAME;
  }

  // Find Frame Start
  for (idx = 0; idx < frmLen - PN532_FRAME_LEN_MIN; idx++) {
    if (p[0] == PN532_PREAMBLE
        && p[1] == PN532_STARTCODE_0
        && p[2] == PN532_STARTCODE_1
       ) {
      break;
    }
    p += 1;
  }
  if (idx == frmLen - PN532_FRAME_LEN_MIN) {
    D("idx - %d\n", idx);
    return PN532_INVALID_FRAME;
  }

  // Special Frame - ACK/NACK
  if (p[3] == 0x00
      && p[4] == 0xFF
     ) {
    return PN532_TFI_ACK;
  }
  if (p[3] == 0xFF
      && p[4] == 0x00
     ) {
    return PN532_TFI_NACK;
  }
  // Extended Frame
  if (p[3] == 0xFF
      && p[4] == 0xFF
     ) {
    return PN532_NOT_SUPPORT;
  }

  // Length and Checksum
  len = p[3];
  if (!len
      || len > PN532_DATABUF_MAX    // FIXME: PN532 support 255 as MAX. Extra limitation here!
     ) {
    DD();
    return PN532_INVALID_FRAME;
  }
  checksum = ~len + 1;
  if (checksum != p[4]) {
    DD();
    return PN532_INVALID_FRAME;
  }
  // Data Checksum
  sum = p[5];                       // TFI
  idx = 6;                          // Position of the next byte data to be processed
  for (i = 0; i < len - 1; i++) {
    sum += p[6 + i];
    idx += 1;
  }
  checksum = ~sum + 1;
  if (checksum != p[idx]) {
    DD();
    return PN532_INVALID_FRAME;
  }

  // Postamble
  // No need to check, just ignore it

  if (ppPacket
      && len >= 1                   // Length include TFI, thus 1 for no data
     ) {
    *ppPacket = (void *)&p[6];
  }
  if (pLen) {
    *pLen = len - 1;                // Not include TFI
  }

  return p[5];
}

uint8 PN532_WakeUp(void)
{
  uint8 res = 0;
  const uint8_t cmd_arr[] =
  {
    0x55, 0x55, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0xFF,
    0x03, 0xFD,
    0xD4, 0x14, 0x01, 0x17, 0x00
  };

  res = mraa_uart_write(uart, cmd_arr, sizeof(cmd_arr));
  LOG("%s res - %d\n", __FUNCTION__, res);
  sleep_ms(100);

  res = PN532_ReadAck();
  sleep_ms(5);
  res = PN532_ReadRsp(resp_buf);
  // Check ACK and Response Frame
  // TODO

  return res;
}

/*
 * +----+----+------+---------+-------+
 * | D4 | 14 | Mode | Timeout | [IRQ] |
 * +----+----+------+---------+-------+
 * Cmd     : 0x14
 * Mode    : 01 - Normal, 02 - Virtual, 03 - Wired
 * Timeout : Only valid for Mode = 02
 * IRQ     : 00 - Disable, 01 - Enable
 */
uint8 PN532_SAMConfig(void)
{
  uint8 res = 0;
  const uint8_t cmd_arr[] =
  {
    0x00, 0x00, 0xff,
    0x05, 0xfb,
    0xd4, 0x14, 0x01, 0x14, 0x01,
    0x02, 0x00
  };

  res = mraa_uart_write(uart, cmd_arr, sizeof(cmd_arr));
  LOG("%s res - %d\n", __FUNCTION__, res);
  sleep_ms(100);

  res = PN532_ReadAck();
  sleep_ms(5);
  res = PN532_ReadRsp(resp_buf);
  // Check ACK and Response Frame
  // TODO

  return PN532_GOOD;
}

/*
 * +----+----+---------+----+------+------+
 * | D4 | 56 | ActPass | BR | Next | More |
 * +----+----+---------+----+------+------+
 */
uint8 PN532_ActiveTarget(void)
{
  uint8 res = 0;
  const uint8_t cmd_arr[] =
  {
    0x00, 0x00, 0xff,
    0x0a, 0xf6,
    0xd4, 0x56, 0x01, 0x00, 0x01,
    0x00, 0xff, 0xff, 0x00, 0x00,
    0xd6, 0x00
  };

  res = mraa_uart_write(uart, cmd_arr, sizeof(cmd_arr));
  LOG("%s res - %d\n", __FUNCTION__, res);
  sleep_ms(50);

  res = PN532_ReadAck();
  sleep_ms(5);
  res = PN532_ReadRsp(resp_buf);
  // Check ACK and Response Frame
  // TODO

  return PN532_GOOD;
}

/*
 * +----+----+--------+--------+--------+----------+-----+----------+
 * | D4 | 60 | PollNr | Period | Type 1 | [Type 2] | ... | [Type N] |
 * +----+----+--------+--------+--------+----------+-----+----------+
 */
uint8 PN532_InAutoPoll(void)
{
  uint8 res = 0;
  const uint8_t cmd_arr[] =
  {
    0x00, 0x00, 0xff,
    0x05, 0xfb,
    0xd4, 0x60, 0xff, 0x01, 0x00,
    0xcc, 0x00
  };

  res = mraa_uart_write(uart, cmd_arr, sizeof(cmd_arr));
  LOG("%s res - %d\n", __FUNCTION__, res);
  sleep_ms(50);

  res = PN532_ReadAck();
  sleep_ms(5);
  res = PN532_ReadRsp(resp_buf);
  // Check ACK and Response Frame
  // TODO

  return PN532_GOOD;
}

uint8 PN532_ReadMifare(void)
{
  uint8 res = 0;
  uint8 cmdData[] = { 0x01, 0x30, 0x00 };

  res = PN532_SendCmd(PN532_CMD_INDATAEXCHANGE, cmdData, sizeof(cmdData), 0);
  LOG("%s res - %d\n", __FUNCTION__, res);
  sleep_ms(50);

  res = PN532_ReadAck();
  sleep_ms(5);
  res = PN532_ReadRsp(resp_buf);
  res = PN532_FrameParser(resp_buf, res, NULL, NULL);
  // Check ACK and Response Frame
  // TODO

  return PN532_GOOD;
}

void PN532_Test(void)
{
  uint8 res = 0;
  uint8 buf[8];
  PN532_InListPassiveTarget_Cmd_t cmd = { 0x01, 0x00, NULL, 0 };
  PN532_InListPassiveTarget_Resp_t resp = { 0x00, 0x00, buf, 0 };

  res = PN532_InListPassiveTarget(&cmd, &resp);
  LOG("InLstPasTg: 0x%02x\r\n", res);
  res = PN532_ReadMifare();
  LOG("ReadMifare: 0x%02x\r\n", res);

  sleep_ms(100);
}

void PN532_Init(void)
{
  int res = 0;

  uart = mraa_uart_init(UART_PORT);

  if (!uart) {
    fprintf(stderr, "UART init fail!\n");
    return ;

  }
  mraa_uart_set_baudrate(uart, UART_BAUDRATE);
  mraa_uart_set_mode(uart, 8, MRAA_UART_PARITY_NONE, 1);
}

int main(void)
{
  uint8 res = 0;
  PN532_FirmwareVersion_t fwVer;

  PN532_Init();

  // LowVbat -> Standby
  PN532_WakeUp();
  PN532_SAMConfig();
#if 0
  PN532_ActiveTarget();
  PN532_InAutoPoll();
#endif

  res = PN532_GetFirmwareVersion(&fwVer);
  LOG("Get_FW_Ver: 0x%02x\r\n", res);

  while (1) {
    PN532_Test();
    sleep(1);
  }
  return 0;
}

/* vim: set ts=2 sw=2 tw=0 list : */