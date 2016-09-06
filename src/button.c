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
        const char *name;
        uint8_t code;
        uint8_t gpio;
} button_io_map_t;

button_io_map_t btn_io_map[] =
{
#if 0
        { "pwr"  , BUTTON_CODE_PWR  , BUTTON_PIN_PWR}  ,
#endif
        { "rst"  , BUTTON_CODE_RST  , BUTTON_PIN_RST}  ,
        { "rec"  , BUTTON_CODE_REC  , BUTTON_PIN_REC}  ,
        { "play" , BUTTON_CODE_PLAY , BUTTON_PIN_PLAY} ,
        { "next" , BUTTON_CODE_NEXT , BUTTON_PIN_NEXT} ,
#if 0
        { "prev" , BUTTON_CODE_PREV , BUTTON_PIN_PREV} ,
        { "left" , BUTTON_CODE_LEFT , BUTTON_PIN_LEFT} ,
        { "right", BUTTON_CODE_RIGHT, BUTTON_PIN_RIGHT} ,
#endif
};

#define BTN_NUM     (sizeof(btn_io_map) / sizeof(btn_io_map[0]))

static mraa_gpio_context btn_gpio[BTN_NUM];

static button_click_status_t sBtnClkStatus = { 0, 0, 0 };

static struct ubus_context *ctx;
static struct blob_buf b;

static int
button_status(struct ubus_context *ctx, struct ubus_object *obj,
                struct ubus_request_data *req, const char *method,
                struct blob_attr *msg)
{
        void *arr;
        void *tbl;
        uint8_t i = 0;

        blob_buf_init(&b, 0);
        arr = blobmsg_open_array(&b, "status");

        for (i = 0; i < BTN_NUM; i++) {
                tbl = blobmsg_open_table(&b, NULL);
                blobmsg_add_string(&b, "name", btn_io_map[i].name);
                blobmsg_add_u16(&b, "gpio", btn_io_map[i].gpio);
                blobmsg_add_u16(&b, "value", mraa_gpio_read(btn_gpio[i]));
                blobmsg_close_table(&b, tbl);

        }
        blobmsg_close_array(&b, arr);

        ubus_send_reply(ctx, req, b.head);

        return 0;
}

enum {
        GET_ID,
        GET_NAME,
        __GET_MAX,
};

static const struct blobmsg_policy button_get_policy[__GET_MAX] = {
        [GET_ID] = { .name = "id", .type = BLOBMSG_TYPE_INT32 },
        [GET_NAME] = { .name = "name", .type = BLOBMSG_TYPE_STRING },
};

static int
button_get(struct ubus_context *ctx, struct ubus_object *obj,
                struct ubus_request_data *req, const char *method,
                struct blob_attr *msg)
{
        struct blob_attr *tb[__GET_MAX];
        uint8_t i = 0;

        blobmsg_parse(button_get_policy, __GET_MAX, tb, blob_data(msg), blob_len(msg));

        if (!tb[GET_ID] && !tb[GET_NAME]) {
                // Check if id valid
                return UBUS_STATUS_INVALID_ARGUMENT;
        }

        for (i = 0; i < BTN_NUM; i++) {
                if (tb[GET_ID] && blobmsg_get_u32(tb[GET_ID]) == btn_io_map[i].gpio) {
                        break;
                }
                if (tb[GET_NAME] && !strcmp(blobmsg_get_string(tb[GET_NAME]), btn_io_map[i].name)) {
                        break;
                }
        }

        if (i >= BTN_NUM) {
                // Not found the quest button by name
                return UBUS_STATUS_INVALID_ARGUMENT;
        }

        blob_buf_init(&b, 0);
        blobmsg_add_string(&b, "name", btn_io_map[i].name);
        blobmsg_add_u16(&b, "gpio", btn_io_map[i].gpio);
        blobmsg_add_u16(&b, "value", mraa_gpio_read(btn_gpio[i]));
        ubus_send_reply(ctx, req, b.head);

        return 0;
}

static const struct ubus_method button_methods[] = {
        { .name = "status" , .handler = button_status } ,
        UBUS_METHOD("get"    , button_get    , button_get_policy)    ,
};

static struct ubus_object_type button_object_type = UBUS_OBJECT_TYPE("key", button_methods);

static struct ubus_object button_object = {
        .name = "key",
        .type = &button_object_type,
        .methods = button_methods,
        .n_methods = ARRAY_SIZE(button_methods),
};

static void button_dispatch_event(uint8_t code, uint8_t action)
{
        // TODO
}

static uint16_t btn_status_bits = -1;

static void button_isr(void *param)
{
        void *arr;
        void *tbl;
        uint8_t i = 0;
        uint16_t btn_bits = 0;

        //LOG("%s\n", __FUNCTION__);

        // Simply to scan all button status.
        // Not necessary, do like this just to be make it simple.
        // TO BE FURTHER OPTIMZED!
        // TODO
        for (i = 0; i < BTN_NUM; i++) {
                btn_bits |= (mraa_gpio_read(btn_gpio[i]) << i);
        }

        // Check which button status has changed
        if (!(btn_status_bits ^ btn_bits)) {
                return ;
        }

        blob_buf_init(&b, 0);
        arr = blobmsg_open_array(&b, "status");

        for (i = 0; i < BTN_NUM; i++) {
                if ((btn_status_bits & (1 << i)) ^ (btn_bits & (1 << i))) {
                        LOG("BTN %d %s\n", i, (btn_bits & (1 << i)) ? "RELEASE" : "PRESSED");
                        // Notify subscriber
                        // TODO
                        tbl = blobmsg_open_table(&b, NULL);
                        blobmsg_add_string(&b, "name", btn_io_map[i].name);
                        blobmsg_add_u16(&b, "gpio", btn_io_map[i].gpio);
                        blobmsg_add_u16(&b, "value", mraa_gpio_read(btn_gpio[i]));
                        blobmsg_close_table(&b, tbl);
                }
        }
        btn_status_bits = btn_bits;

        blobmsg_close_array(&b, arr);
        ubus_notify(ctx,  &button_object, "button", b.head, -1);
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

static void button_main(void)
{
        int ret;

        button_init();

        ret = ubus_add_object(ctx, &button_object);
        if (ret) {
                fprintf(stderr, "Failed to add object: %s\n", ubus_strerror(ret));
        }
        uloop_run();

#if 0
        while (1) {
                button_test();
                sleep(2);
        }
#endif
}

int main(int argc, char **argv)
{
        const char *ubus_socket = NULL;
        int ch;

        while ((ch = getopt(argc, argv, "cs:")) != -1) {
                switch (ch) {
                        case 's':
                                ubus_socket = optarg;
                                break;
                        default:
                                break;
                }
        }

        argc -= optind;
        argv += optind;

        uloop_init();
        signal(SIGPIPE, SIG_IGN);

        ctx = ubus_connect(ubus_socket);
        if (!ctx) {
                fprintf(stderr, "Failed to connect to ubus\n");
                return -1;
        }

        ubus_add_uloop(ctx);

        button_main();

        ubus_free(ctx);
        uloop_done();

        return 0;
}

/* vim: set ts=8 sw=8 tw=0 list : */
