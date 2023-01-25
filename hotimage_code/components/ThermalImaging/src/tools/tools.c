#include "tools.h"
#include <esp_heap_caps.h>
#include <esp_chip_info.h>
#include <esp_spi_flash.h>
#include <esp32/spiram.h>

void printfESPChipInfo(void)
{
    // 显示有关 CPU 的信息
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);

    printf("ESP32 chip model %d revision %d (%d CPU cores, WiFi%s%s)\n", chip_info.model, chip_info.revision, chip_info.cores, (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "", (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");
    printf("%d MB %s SPI FLASH\n", spi_flash_get_chip_size() / (1024 * 1024), (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

#ifdef CONFIG_ESP32_SPIRAM_SUPPORT
    printf("%d MB SPI PSRAM\n", esp_spiram_get_size() / (1024 * 1024));
    printf("Available Memory:\r\n");
    printf("Internal Memory: %d K\r\n", heap_caps_get_free_size(MALLOC_CAP_INTERNAL) / 1024 / 8);
    printf("External Memory: %d K\r\n", heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / 1024 / 8);
#endif
}
