#include "thermalimaging.h"

#define AVERAGE_COUNT 10

#ifdef CONFIG_ESP32_IIC_SHT31

static SAFilterHandle_t* pFilter_Temperature = NULL;
static SAFilterHandle_t* pFilter_Humidity = NULL;

/**
 * @brief 获取温度值
 *
 * @return float
 */
float sht31_getTemperature()
{
    return GetSAFiterRes(pFilter_Temperature);
}

/**
 * @brief 获取湿度值
 *
 * @return float
 */
float sht31_getHumidity()
{
    return GetSAFiterRes(pFilter_Humidity);
}

// 初始化sht31
void sht31_task(void* arg)
{
    float temperature, humidity;
    uint16_t temperature_raw, humidity_raw;

    pFilter_Temperature = SlipAveFilterCreate(AVERAGE_COUNT);
    pFilter_Humidity = SlipAveFilterCreate(AVERAGE_COUNT);

    sht31_init();
    if (1 == sht31_soft_reset()) {
        goto err;
    }
    vTaskDelay(100 / portTICK_RATE_MS);

    while (1) {
        sht31_single_command(SHT31_BOOL_TRUE);
        vTaskDelay(20 / portTICK_RATE_MS);

        sht31_single_read(&temperature_raw, &temperature, &humidity_raw, &humidity);
        AddSAFiterRes(pFilter_Temperature, temperature);
        AddSAFiterRes(pFilter_Humidity, humidity);

        vTaskDelay(1 * 1000 / portTICK_RATE_MS);
    }

err:
    printf("Init SHT31 Error\n");
    vTaskDelete(NULL);
}
#endif // CONFIG_ESP32_IIC_SHT31