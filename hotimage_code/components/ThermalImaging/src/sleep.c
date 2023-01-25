#include "sleep.h"
#include "dispcolor.h"
#include "driver/adc.h"
#include "driver/rtc_io.h"
// #include "esp32/ulp.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "soc/rtc.h"
#include "soc/rtc_cntl_reg.h"
#include "soc/sens_reg.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

// static RTC_DATA_ATTR struct timeval sleep_enter_time;

/**
 * @brief ESP32进入睡眠模式
 *
 */
void Deep_Sleep_Run()
{
#if 0
    struct timeval now;
    gettimeofday(&now, NULL);

    switch (esp_sleep_get_wakeup_cause()) {
    case ESP_SLEEP_WAKEUP_EXT1: {
        uint64_t wakeup_pin_mask = esp_sleep_get_ext1_wakeup_status();
        if (wakeup_pin_mask != 0) {
            int pin = __builtin_ffsll(wakeup_pin_mask) - 1;
            printf("Wake Up From GPIO%d\n", pin);
        } else {
            printf("Wake Up From GPIO\n");
        }
        break;
    }

    case ESP_SLEEP_WAKEUP_UNDEFINED:
    default:
        printf("Not Deep Sleep Reset\n");
    }

    vTaskDelay(1000 / portTICK_PERIOD_MS);

    const int ext_wakeup_pin = 38;
    const uint64_t ext_wakeup_pin_mask = 1ULL << ext_wakeup_pin;

    printf("In Pin GPIO%d Enable EXT1 Wakeup On\n", ext_wakeup_pin);
    esp_sleep_enable_ext1_wakeup(ext_wakeup_pin_mask, ESP_EXT1_WAKEUP_ALL_LOW);

    // Isolate GPIO12 pin from external circuits. This is needed for modules
    // which have an external pull-up resistor on GPIO12 (such as ESP32-WROVER)
    // to minimize current consumption.
    rtc_gpio_isolate(GPIO_NUM_0);
    rtc_gpio_isolate(GPIO_NUM_12);
    rtc_gpio_isolate(GPIO_NUM_25);
    rtc_gpio_isolate(GPIO_NUM_34);
    rtc_gpio_isolate(GPIO_NUM_35);
    rtc_gpio_isolate(GPIO_NUM_36);

    dispcolor_SetBrightness(0);
    dispcolor_DisplayOff();
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    gpio_hold_en(GPIO_NUM_5);
    gpio_deep_sleep_hold_en();

    printf("Enter Deep Sleep\n");
    gettimeofday(&sleep_enter_time, NULL);

    esp_deep_sleep_start();
#endif
}

// TODO 这里编译不通过 先注释