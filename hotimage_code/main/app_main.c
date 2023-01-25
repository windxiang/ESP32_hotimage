#include "thermalimaging.h"
#include <esp32/himem.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/semphr.h>

EventGroupHandle_t* pHandleEventGroup = NULL;
SemaphoreHandle_t pSPIMutex = NULL;

void app_main()
{
#if 0
    gpio_num_t gpio[] = { GPIO_NUM_32 };
    for (int i = 0; i < 1; i++) {
        PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[gpio[i]], PIN_FUNC_GPIO);
        gpio_reset_pin(gpio[i]);
        gpio_set_direction(gpio[i], GPIO_MODE_OUTPUT);
    }

    // 显示有关 CPU 的信息
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    printf("ESP32 chip model %d revision %d (%d CPU cores, WiFi%s%s)\n", chip_info.model, chip_info.revision, chip_info.cores, (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "", (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");
    printf("%d MB %s SPI FLASH\n", spi_flash_get_chip_size() / (1024 * 1024), (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");
    printf("%d MB SPI PSRAM\n", esp_spiram_get_size() / (1024 * 1024));

    if (esp_spiram_test()) {
        printf("SPI SRAM memory test OK\n");
    } else {
        printf("SPI SRAM memory test fail \n");
    }

    while (1) {
        printf("0\r\n");
        for (int i = 0; i < 1; i++) {
            gpio_set_level(gpio[i], 0);
        }
        vTaskDelay(2 * 1000 / portTICK_RATE_MS);

        printf("1\r\n");
        for (int i = 0; i < 1; i++) {
            gpio_set_level(gpio[i], 1);
        }
        vTaskDelay(2 * 1000 / portTICK_RATE_MS);
    }
#endif
    pSPIMutex = xSemaphoreCreateMutex();

    gpio_hold_dis(GPIO_NUM_5);

    // 初始化显示
    dispcolor_Init();

    printfESPChipInfo();

    // 初始化配置
    int err = settings_storage_init();
    settings_read_all();
    if (err) {
        console_printf(MsgWarning, "Error Reading Settings !\r\n");
    }

    // 设置LCD背光
    dispcolor_SetBrightness(settingsParms.LcdBrightness);

#ifdef CONFIG_ESP32_IIC_SUPPORT
    // 初始化 iic
    I2CInit(CONFIG_IIC_IONUM_SDA, CONFIG_IIC_IONUM_SCL, CONFIG_ESP32_IIC_CLOCK, I2C_MODE_MASTER);
    i2c_general_reset();
    // i2c_master_scan();
#endif // CONFIG_ESP32_IIC_SUPPORT

    // 创建任务
    pHandleEventGroup = xEventGroupCreate();

#ifdef CONFIG_ESP32_WIFI_SUPPORT
    // wifi 内存需求很大,所以先进行初始化.防止内存不足
    xTaskCreatePinnedToCore(wifi_task, "wifi", 1024 * 10, NULL, 5, NULL, tskNO_AFFINITY);
    vTaskDelay(1000 / portTICK_RATE_MS); // 加入延时 等待初始化完成
#endif

    xTaskCreatePinnedToCore(adc_task, "adc", 1024 * 3, NULL, 5, NULL, tskNO_AFFINITY);
    xTaskCreatePinnedToCore(render_task, "render", 1024 * 20, NULL, 5, NULL, 1);

#ifdef CONFIG_ESP32_IIC_MLX90640
    xTaskCreatePinnedToCore(mlx90640_task, "mlx90640", 1024 * 5, NULL, 5, NULL, tskNO_AFFINITY);
#endif // CONFIG_ESP32_IIC_MLX90640

    xTaskCreatePinnedToCore(buttons_task, "buttons", 1024, NULL, 6, NULL, tskNO_AFFINITY);

#ifdef CONFIG_ESP32_IIC_SHT31
    xTaskCreatePinnedToCore(sht31_task, "sht31", 1024, NULL, 5, NULL, tskNO_AFFINITY);
#endif // CONFIG_ESP32_IIC_SHT31

#ifdef CONFIG_ESP32_SPI_SDCARD
    sdcard_task(NULL);
#endif

    while (1) {
        vTaskDelay(60 * 1000 / portTICK_RATE_MS);
        // printf("Internal Memory: %d K\r\n", heap_caps_get_free_size(MALLOC_CAP_INTERNAL) / 1024 / 8);
        // printf("External Memory: %d K\r\n", heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / 1024 / 8);
    }
}
