#include <stdio.h>
#include <unistd.h>
#include <signal.h>

#include <libubox/blobmsg_json.h>
#include "libubus.h"
#include "mraa.h"

#include "led.h"

#define LOG_TAG         "Led"

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

#define DEBUG_LED_LOG   0

#if DEBUG_LED_LOG
#define LOGD_LED        LOGD
#else
#define LOGD_LED        __LOGD
#endif

typedef struct {
        const char *name;
        uint8_t code;
        uint8_t gpio;
} led_io_map_t;

led_io_map_t led_io_map[] =
{
        { "status"  , LED_CODE_STATUS  , LED_PIN_STATUS  } ,
        { "alert"   , LED_CODE_ALERT   , LED_PIN_ALERT   } ,
        { "battery" , LED_CODE_BATTERY , LED_PIN_BATTERY } ,
};

#define LED_NUM     (sizeof(led_io_map) / sizeof(led_io_map[0]))

static mraa_gpio_context led_gpio[LED_NUM];

static struct ubus_context *ctx;
static struct blob_buf b;

static int
led_status(struct ubus_context *ctx, struct ubus_object *obj,
                struct ubus_request_data *req, const char *method,
                struct blob_attr *msg)
{
        void *arr;
        void *tbl;
        uint8_t i = 0;

        blob_buf_init(&b, 0);
        arr = blobmsg_open_array(&b, "status");

        for (i = 0; i < LED_NUM; i++) {
                tbl = blobmsg_open_table(&b, NULL);
                blobmsg_add_string(&b, "name", led_io_map[i].name);
                blobmsg_add_u16(&b, "gpio", led_io_map[i].gpio);
                blobmsg_add_u16(&b, "value", mraa_gpio_read(led_gpio[i]));
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

static const struct blobmsg_policy led_get_policy[__GET_MAX] = {
        [GET_ID] = { .name = "id", .type = BLOBMSG_TYPE_INT32 },
        [GET_NAME] = { .name = "name", .type = BLOBMSG_TYPE_STRING },
};

static int
led_get(struct ubus_context *ctx, struct ubus_object *obj,
                struct ubus_request_data *req, const char *method,
                struct blob_attr *msg)
{
        struct blob_attr *tb[__GET_MAX];
        uint8_t i = 0;

        blobmsg_parse(led_get_policy, __GET_MAX, tb, blob_data(msg), blob_len(msg));

        if (!tb[GET_ID] && !tb[GET_NAME]) {
                // Check if id valid
                return UBUS_STATUS_INVALID_ARGUMENT;
        }

        for (i = 0; i < LED_NUM; i++) {
                if (tb[GET_ID] && blobmsg_get_u32(tb[GET_ID]) == led_io_map[i].gpio) {
                        break;
                }
                if (tb[GET_NAME] && !strcmp(blobmsg_get_string(tb[GET_NAME]), led_io_map[i].name)) {
                        break;
                }
        }

        if (i >= LED_NUM) {
                // Not found the quest led by name
                return UBUS_STATUS_INVALID_ARGUMENT;
        }

        // {
        //      "name": "xxx"
        //      "gpio": 16
        //      "value": 1
        // }
        blob_buf_init(&b, 0);
        blobmsg_add_string(&b, "name", led_io_map[i].name);
        blobmsg_add_u16(&b, "gpio", led_io_map[i].gpio);
        blobmsg_add_u16(&b, "value", mraa_gpio_read(led_gpio[i]));
        ubus_send_reply(ctx, req, b.head);

        return 0;
}

enum {
        SET_ID,
        SET_ACTION,
        __SET_MAX,
};

static const struct blobmsg_policy led_set_policy[__SET_MAX] = {
        [SET_ID] = { .name = "id", .type = BLOBMSG_TYPE_INT32 },
        [SET_ACTION] = { .name = "action", .type = BLOBMSG_TYPE_INT32 },
};

static int
led_set(struct ubus_context *ctx, struct ubus_object *obj,
                struct ubus_request_data *req, const char *method,
                struct blob_attr *msg)
{
        struct blob_attr *tb[__SET_MAX];
        uint8_t i = 0;

        blobmsg_parse(led_set_policy, __SET_MAX, tb, blob_data(msg), blob_len(msg));

        if (!tb[SET_ID] || !tb[SET_ACTION]) {
                // Check if id valid
                return UBUS_STATUS_INVALID_ARGUMENT;
        }

        for (i = 0; i < LED_NUM; i++) {
                if (tb[SET_ID] && blobmsg_get_u32(tb[SET_ID]) == led_io_map[i].gpio) {
                        break;
                }
        }

        if (i >= LED_NUM) {
                // Not found the quest led by name
                return UBUS_STATUS_INVALID_ARGUMENT;
        }

        switch (blobmsg_get_u32(tb[SET_ACTION])) {
                case 0:
                        // OFF
                        mraa_gpio_write(led_gpio[i], 0);
                        break;
                case 1:
                        mraa_gpio_write(led_gpio[i], 1);
                        // ON
                        break;
                case 2:
                        // FLASH
                        // TODO
                        break;
                default:
                        break;
        }


        blob_buf_init(&b, 0);
        blobmsg_add_string(&b, "name", led_io_map[i].name);
        blobmsg_add_u16(&b, "gpio", led_io_map[i].gpio);
        blobmsg_add_u16(&b, "value", mraa_gpio_read(led_gpio[i]));
        ubus_send_reply(ctx, req, b.head);

        return 0;
}

static const struct ubus_method led_methods[] = {
        { .name = "status" , .handler = led_status } ,
        UBUS_METHOD("get"    , led_get    , led_get_policy)    ,
        UBUS_METHOD("set"    , led_set    , led_set_policy)    ,
};

static struct ubus_object_type led_object_type = UBUS_OBJECT_TYPE("led", led_methods);

static struct ubus_object led_object = {
        .name = "led",
        .type = &led_object_type,
        .methods = led_methods,
        .n_methods = ARRAY_SIZE(led_methods),
};

static void led_dispatch_event(uint8_t code, uint8_t action)
{
        // TODO
}

static uint16_t led_status_bits = 0;

int led_init(void)
{
        uint8_t i = 0;

        for (i = 0; i < LED_NUM; i++) {
                led_gpio[i] = mraa_gpio_init(led_io_map[i].gpio);
                if (!led_gpio[i]) {
                        ERR("Init GPIO %d FAIL\n", led_io_map[i].gpio);
                        return -1;
                }
                mraa_gpio_dir(led_gpio[i], MRAA_GPIO_OUT);
        }

        return 0;
}

void led_test(void)
{
        // TODO
        printf("%s\n", __FUNCTION__);
}

static void led_main(void)
{
        int ret;

        led_init();

        ret = ubus_add_object(ctx, &led_object);
        if (ret) {
                fprintf(stderr, "Failed to add object: %s\n", ubus_strerror(ret));
        }
        uloop_run();

#if 0
        while (1) {
                led_test();
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

        led_main();

        ubus_free(ctx);
        uloop_done();

        return 0;
}

/* vim: set ts=8 sw=8 tw=0 list : */
