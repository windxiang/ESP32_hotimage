#include "driver/adc.h"
#include "driver/gpio.h"
#include "esp_adc_cal.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "soc/gpio_struct.h"
#include "thermalimaging.h"
#include <string.h>

#define AVERAGE_ADC_CHARGE 5 // 是否充电
#define AVERAGE_ADC_BATVOL 64 // 电池电压

static SAFilterHandle_t* pFilter_ADC_charge = NULL; // 是否充电
static SAFilterHandle_t* pFilter_ADC_vol = NULL; // 电池电压

#define UPPER_DIVIDER 442 // 电阻值
#define LOWER_DIVIDER 160 // 电阻值
#define DEFAULT_VREF 1100 // Use adc2_vref_to_gpio() to obtain a better estimate

static esp_adc_cal_characteristics_t* adc_chars = NULL;
static const adc_channel_t CHANNEL_BATCHARGE = ADC_CHANNEL_6; // GPIO34 ADC1 CHANNEL6 是否充电
static const adc_channel_t CHANNEL_BATVOL = ADC_CHANNEL_7; // GPIO35 ADC1 CHANNEL7 电池电压
static const adc_atten_t atten = ADC_ATTEN_DB_6;
static const adc_unit_t unit = ADC_UNIT_1;

// 检查ADC校准值是否烧录刀eFuse
static void check_efuse()
{
    // Check TP is burned into eFuse
    if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_TP) == ESP_OK) {
        printf("eFuse Two Point: Support\r\n");
    } else {
        printf("eFuse Two Point: Not Support\r\n");
    }

    // Check Vref is burned into eFuse
    if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_VREF) == ESP_OK) {
        printf("eFuse Vref: Support\r\n");
    } else {
        printf("eFuse Vref: Not Support\r\n");
    }
}

static void print_char_val_type(esp_adc_cal_value_t val_type)
{
    if (val_type == ESP_ADC_CAL_VAL_EFUSE_TP) {
        printf("Use Two Point Value\r\n");
    } else if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF) {
        printf("Use eFuse Vref\r\n");
    } else {
        printf("Use Default Vref\r\n");
    }
}

// 获得电池电压
uint32_t getBatteryVoltage()
{
    if (NULL == adc_chars) {
        return 0;
    }
    float adc_reading = GetSAFiterRes(pFilter_ADC_vol);

    // 电池电压计算
    uint32_t voltage = esp_adc_cal_raw_to_voltage((uint32_t)adc_reading, adc_chars) * (LOWER_DIVIDER + UPPER_DIVIDER) / LOWER_DIVIDER;
    return voltage;
}

// 判断是否充电中
int8_t getBatteryCharge()
{
    if (NULL == pFilter_ADC_charge) {
        return -1;
    }
    float adc_reading = GetSAFiterRes(pFilter_ADC_charge);
    return (int8_t)adc_reading;
}

/**
 * @brief 进行一次AD采样转换
 *
 * @return float
 */
static float ADCGetVol()
{
    float adc_reading = 0.0f;

    if (unit == ADC_UNIT_1) {
        adc_reading += adc1_get_raw((adc1_channel_t)CHANNEL_BATVOL);

    } else {
        int raw;
        adc2_get_raw((adc2_channel_t)CHANNEL_BATVOL, ADC_WIDTH_BIT_12, &raw);
        adc_reading += raw;
    }

    return adc_reading;
}

static float ADCGetCharge()
{
    uint32_t raw = adc1_get_raw((adc1_channel_t)CHANNEL_BATCHARGE);
    return (float)raw;
}

/**
 * @brief 初始化ADC外设
 *
 * @return esp_err_t
 */
static esp_err_t init_adc()
{
    esp_err_t err = ESP_OK;

    // console_printf(MsgInfo, "ADC initialization for measuring VBAT (ADC1, CHANNEL_BATVOL=%d, 0 db, Vref=%d mV)\r\n", CHANNEL_BATVOL, DEFAULT_VREF);

    // 检查Two Point或Vref是否烧入eFuse
    check_efuse();

    // 配置 ADC1 精度
    err = adc1_config_width(ADC_WIDTH_BIT_12);
    if (err != ESP_OK) {
        // console_printf(MsgError, "Function adc1_config_width() returns error code: %d\r\n", err);
        return err;
    }

    // 配置通道 设置采样衰减
    err = adc1_config_channel_atten(CHANNEL_BATVOL, atten);
    if (err != ESP_OK) {
        return err;
    }

    err = adc1_config_channel_atten(CHANNEL_BATCHARGE, atten);
    if (err != ESP_OK) {
        return err;
    }

    // 校准
    adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
    esp_adc_cal_value_t val_type = esp_adc_cal_characterize(unit, atten, ADC_WIDTH_BIT_12, DEFAULT_VREF, adc_chars);
    print_char_val_type(val_type);

    // console_printf(MsgInfo, "The ADC used for VBAT measurement has been initialized\r\n");

    return err;
}

void adc_task(void* arg)
{
    // ADC 初始化
    if (ESP_OK != init_adc()) {
        goto error;
    }

    pFilter_ADC_vol = SlipAveFilterCreate(AVERAGE_ADC_BATVOL);
    pFilter_ADC_charge = SlipAveFilterCreate(AVERAGE_ADC_CHARGE);

    // 是否充电
    for (uint16_t i = 0; i < AVERAGE_ADC_CHARGE; i++) {
        AddSAFiterRes(pFilter_ADC_charge, ADCGetVol());
        vTaskDelay(10 / portTICK_RATE_MS);
    }

    // 当前电池电压
    for (uint16_t i = 0; i < AVERAGE_ADC_BATVOL; i++) {
        AddSAFiterRes(pFilter_ADC_vol, ADCGetVol());
        vTaskDelay(50 / portTICK_RATE_MS);
    }

    while (1) {
        AddSAFiterRes(pFilter_ADC_vol, ADCGetVol());
        AddSAFiterRes(pFilter_ADC_charge, ADCGetCharge());
        vTaskDelay(200 / portTICK_RATE_MS);
    }

error:
    printf("Error ADC init Tasks\r\n");
}

// TODO ESP32好像不支持DMA方式读取ADC的值？
