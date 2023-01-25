#include "thermalimaging.h"
#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// 3个按钮GPIO定义
#define BUTTON_UP_PIN 37
#define BUTTON_CENTER_PIN 38
#define BUTTON_DOWN_PIN 36

//
#define LONG_PRESS_STAGE1 8 // 单位100ms
#define LONG_PRESS_STAGE2 50 // 单位100ms
#define STAGE2_ACCELERATION 4

#define BUTTON_IO_NUM 0
#define BUTTON_ACTIVE_LEVEL 0
#define BUTTON_NUM 3

// 消抖时间
#define BUTTON_DEBOUNCE_TIME 2 // 单位100ms

typedef struct {
    uint8_t pin; // 按钮GPIO
    uint32_t pressEvent; // 按下事件
    uint32_t holdEvent; // 长按事件
    bool wasPressed;
    bool wasHeldDown; // 长按标记位
    uint32_t counter; // 按下计数器
    uint8_t stageCounter;
} sGPIOButton;

/**
 * @brief 按钮初始化
 *
 */
void buttons_init()
{
    gpio_config_t io_conf;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = BIT64(BUTTON_UP_PIN) | BIT64(BUTTON_CENTER_PIN) | BIT64(BUTTON_DOWN_PIN);
    io_conf.pull_down_en = false;
    io_conf.pull_up_en = false;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&io_conf);
}

// 发送按下事件
static void sendButtonEvent(uint32_t event)
{
    if (NULL != pHandleEventGroup)
        xEventGroupSetBits(pHandleEventGroup, event);
}

// 判断按键释放(click)
static uint8_t isSinglePressReleased(sGPIOButton* button, bool pressed)
{
    return !button->wasHeldDown && button->wasPressed && !pressed;
}

/**
 * @brief 处理按键响应
 *
 * @param button
 */
static void handleStateChangeOf(sGPIOButton* button)
{
    // 判断按下
    bool pressed = gpio_get_level(button->pin) == 0;

    if (pressed && button->wasPressed) {
        if ((++button->counter > LONG_PRESS_STAGE2) || //
            (button->counter > LONG_PRESS_STAGE1 && ++button->stageCounter == STAGE2_ACCELERATION)) {
            // 长按
            button->stageCounter = 0;
            button->wasHeldDown = true;
            sendButtonEvent(button->holdEvent);
        }

    } else if (isSinglePressReleased(button, pressed)) {
        // 按下
        button->counter = button->stageCounter = 0;
        sendButtonEvent(button->pressEvent);
    }

    if (!pressed)
        button->wasHeldDown = false;
    button->wasPressed = pressed;
}

void buttons_task(void* arg)
{
    // 按钮初始化
    buttons_init();

    sGPIOButton buttonUp = { BUTTON_UP_PIN, RENDER_ShortPress_Up, RENDER_Hold_Up, false, false, 0, 0 };
    sGPIOButton buttonCenter = { BUTTON_CENTER_PIN, RENDER_ShortPress_Center, RENDER_Hold_Center, false, false, 0, 0 };
    sGPIOButton buttonDown = { BUTTON_DOWN_PIN, RENDER_ShortPress_Down, RENDER_Hold_Down, false, false, 0, 0 };

    while (1) {
        handleStateChangeOf(&buttonUp);
        handleStateChangeOf(&buttonCenter);
        handleStateChangeOf(&buttonDown);

        vTaskDelay(100 / portTICK_RATE_MS);
    }
}
