#include <stdio.h>
#include <unistd.h>
#include <signal.h>

#include <libubox/blobmsg_json.h>
#include "libubus.h"
#include "mraa.h"

#include "button.h"

#define LOG_TAG         "Btn"

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

#define ERR(fmt, arg...)                fprintf(stderr, fmt, ##arg)

#define DEBUG_BTN_LED   0
#define DEBUG_BTN_LOG   0

#if DEBUG_BTN_LOG
#define LOGD_BTN        LOGD
#else
#define LOGD_BTN        __LOGD
#endif

typedef struct {
        uint8_t code;
        uint8_t gpio;
} button_io_map_t;

button_io_map_t btn_io_map[] =
{
        { BUTTON_CODE_PWR  , BUTTON_PIN_PWR}  ,
        { BUTTON_CODE_RST  , BUTTON_PIN_RST}  ,
        { BUTTON_CODE_REC  , BUTTON_PIN_REC}  ,
        { BUTTON_CODE_PLAY , BUTTON_PIN_PLAY} ,
        { BUTTON_CODE_PREV , BUTTON_PIN_PREV} ,
        { BUTTON_CODE_NEXT , BUTTON_PIN_NEXT} ,
};

#define BTN_NUM     (sizeof(btn_io_map) / sizeof(btn_io_map[0]))

static mraa_gpio_context btn_gpio[BTN_NUM];

static button_click_status_t sBtnClkStatus = { 0, 0, 0 };

static void button_dispatch_event(uint8_t code, uint8_t action)
{
        // TODO
}

static void button_isr(void *param)
{
        LOG("%s\n", __FUNCTION__);

        // TODO
}

int button_init(void)
{
        uint8_t i = 0;

        for (i = 0; i < BTN_NUM; i++) {
                btn_gpio[i] = mraa_gpio_init(btn_io_map[i].gpio);
                if (!btn_gpio[i]) {
                        ERR("Init GPIO %d FAIL\n", btn_io_map[i].gpio);
                        return -1;
                }
                mraa_gpio_dir(btn_gpio[i], MRAA_GPIO_IN);
                mraa_gpio_isr(btn_gpio[i], MRAA_GPIO_EDGE_BOTH, button_isr, NULL);
        }

        return 0;
}

void button_press_ind(void)
{
        // TODO
#if 0
        if (BUTTON_PIN_PWR) {
                // Button released
                LOGD_BTN(LOG_TAG, "R\r\n");
                osal_stop_timerEx(muaTaskId, MUA_TASK_EVT_BUTTON_LP);
                if (sBtnClkStatus.clkDetecting) {
                        sBtnClkStatus.clkCnt += 1;
                }
                Button_DispatchEvent(BUTTON_CODE_PWR, BUTTON_ACTION_RELEASED);
        } else {
                // Button pressed
                LOGD_BTN(LOG_TAG, "P\r\n");
                osal_start_timerEx(muaTaskId, MUA_TASK_EVT_BUTTON_LP, BUTTON_LONGPRESS_TIME);
                if (!sBtnClkStatus.clkDetecting) {
                        sBtnClkStatus.clkDetecting = 1;
                        sBtnClkStatus.clkCnt = 0;
                        osal_start_timerEx(muaTaskId, MUA_TASK_EVT_BUTTON_CLK, BUTTON_MULTIPLECLICK_TIME);
                }
                Button_DispatchEvent(BUTTON_CODE_PWR, BUTTON_ACTION_PRESSED);
        }
#endif
}

void button_longpress_ind(void)
{
        // TODO
#if 0
        LOGD_BTN(LOG_TAG, "LP\r\n");
        osal_stop_timerEx(muaTaskId, MUA_TASK_EVT_BUTTON_LP);
        // Clear click detection to avoid long press and double/trible click event at same time
        osal_stop_timerEx(muaTaskId, MUA_TASK_EVT_BUTTON_CLK);
        *(uint8_t *)&sBtnClkStatus = 0;
        Button_DispatchEvent(BUTTON_CODE_PWR, BUTTON_ACTION_LONGPRESS);
#endif
}

void button_click_monitor(void)
{
#if 0
        osal_stop_timerEx(muaTaskId, MUA_TASK_EVT_BUTTON_CLK);
        if (!sBtnClkStatus.clkDetecting) {
                return ;
        }

        // Can also extend to detect trible click
        // TODO
        LOGD_BTN(LOG_TAG, "%dC\r\n", sBtnClkStatus.clkCnt);
        if (sBtnClkStatus.clkCnt >= 3) {
                // 3 Click
                Button_DispatchEvent(BUTTON_CODE_PWR, BUTTON_ACTION_TRIBLE_CLICK);
        } else if (sBtnClkStatus.clkCnt >= 2) {
                // 2 Click
                Button_DispatchEvent(BUTTON_CODE_PWR, BUTTON_ACTION_DOUBLE_CLICK);
        } else {
                // 1/0 Click
        }

        // Clear finally
        sBtnClkStatus.clkDetecting = 0;
        sBtnClkStatus.clkCnt = 0;
#endif
}

void button_test(void)
{
        // TODO
        printf("%s\n", __FUNCTION__);
}

int main(void)
{
        while (1) {
                button_test();
                sleep(2);
        }
        return 0;
}

/* vim: set ts=8 sw=8 tw=0 list : */
