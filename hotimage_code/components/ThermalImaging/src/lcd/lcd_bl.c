#include "spi_lcd.h"
#include <driver/ledc.h>
#include <math.h>

static const ledc_timer_config_t ledc_timer = {
    .speed_mode = LEDC_LOW_SPEED_MODE, // timer mode
    .duty_resolution = LEDC_TIMER_8_BIT, // resolution of PWM duty
    .timer_num = LEDC_TIMER_1, // timer index
    .freq_hz = 5000, // frequency of PWM signal
    .clk_cfg = LEDC_AUTO_CLK, // Auto select the source clock
};

static const ledc_channel_config_t ledc_channel = {
    .gpio_num = LCD_PIN_NUM_BCKL, // GPIO
    .speed_mode = LEDC_LOW_SPEED_MODE, // 低速模式 最大9.6Khz
    .channel = LEDC_CHANNEL_3,
    .timer_sel = LEDC_TIMER_1,
    .duty = 0,
    .hpoint = 0,
};

/**
 * @brief 设置背光
 *
 * @param value
 */
void lcd_backlight_set(uint16_t value)
{
#ifdef LCD_PIN_NUM_BCKL
    if (value > 100)
        value = 100;

    uint32_t duty = (uint32_t)((float)value / 100 * (float)(pow(2, LEDC_TIMER_8_BIT) - 1));

    // 直接修改PWM
    // ledc_set_duty(ledc_channel.speed_mode, ledc_channel.channel, duty);
    // ledc_update_duty(ledc_channel.speed_mode, ledc_channel.channel);

    // 渐变修改PWM
    ledc_set_fade_with_time(ledc_channel.speed_mode, ledc_channel.channel, duty, 200);
    ledc_fade_start(ledc_channel.speed_mode, ledc_channel.channel, LEDC_FADE_WAIT_DONE);
#endif
}

/**
 * @brief 初始化背光PWM GPIO
 *
 */
void lcd_backlight_init(void)
{
#ifdef LCD_PIN_NUM_BCKL
    // Set configuration of timer0 for high speed channels
    ledc_timer_config(&ledc_timer);
    ledc_channel_config(&ledc_channel);
    ledc_fade_func_install(0);

    lcd_backlight_set(10);
    // ledc_set_fade_with_time(ledc_channel.speed_mode, ledc_channel.channel, 0, 10);
    // ledc_fade_start(ledc_channel.speed_mode, ledc_channel.channel, LEDC_FADE_NO_WAIT);
#endif
}
