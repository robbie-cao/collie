#ifndef __BUTTON_H__
#define __BUTTON_H__

#define BUTTON_PIN_PWR            0     // TBD
#define BUTTON_PIN_RST            14
#define BUTTON_PIN_REC            15
#define BUTTON_PIN_PLAY           16
#define BUTTON_PIN_PREV           17
#define BUTTON_PIN_NEXT           17
#define BUTTON_PIN_LEFT           0
#define BUTTON_PIN_RIGHT          41

#define BUTTON_MULTIPLECLICK_TIME 1000    /* ms */
#define BUTTON_LONGPRESS_TIME     2000    /* ms */

enum {
        BUTTON_CODE_PWR,
        BUTTON_CODE_RST,
        BUTTON_CODE_REC,
        BUTTON_CODE_PLAY,
        BUTTON_CODE_PREV,
        BUTTON_CODE_NEXT,
        BUTTON_CODE_LEFT,
        BUTTON_CODE_RIGHT,

        // More if any
};

enum {
        BUTTON_ACTION_PRESSED = 0,
        BUTTON_ACTION_RELEASED,
        BUTTON_ACTION_LONGPRESS,
        BUTTON_ACTION_DOUBLE_CLICK,
        BUTTON_ACTION_TRIBLE_CLICK,
};

typedef struct {
        uint8_t code;
        uint8_t action;
} button_evt_data_t;

typedef struct {
        uint8_t clk_detecting : 1;
        uint8_t clk_cnt       : 3;
        uint8_t rfu          : 4;
} button_click_status_t;

void button_press_ind(void);
void button_longpress_ind(void);
void button_click_monitor(void);

int button_init(void);

#endif /* __BUTTON_H__ */

/* vim: set ts=8 sw=8 tw=0 list : */
