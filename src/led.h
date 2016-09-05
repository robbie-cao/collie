#ifndef __LED_H__
#define __LED_H__

#define LED_PIN_STATUS              14
#define LED_PIN_ALERT               15
#define LED_PIN_BATTERY             16

enum {
        LED_CODE_STATUS,
        LED_CODE_ALERT,
        LED_CODE_BATTERY,

        // More if any
};

#endif /* __LED_H__ */

/* vim: set ts=8 sw=8 tw=0 list : */
