#include "thermalimaging.h"

#include <driver/gpio.h>
#include <driver/sdmmc_host.h>
#include <driver/sdspi_host.h>
#include <esp_vfs_fat.h>
#include <sdmmc_cmd.h>

// SD卡挂载句柄
static sdmmc_card_t* pCardHandler = NULL;

#define SD_PIN_NUM_MOSI CONFIG_SDCARD_IONUM_MOSI
#define SD_PIN_NUM_MISO CONFIG_SDCARD_IONUM_MISO
#define SD_PIN_NUM_CLK CONFIG_SDCARD_IONUM_CLK
#define SD_PIN_NUM_CS CONFIG_SDCARD_IONUM_CS
#define SD_PIN_NUM_CD CONFIG_SDCARD_IONUM_CD // GPIO插入中断

#ifdef CONFIG_SDCARD_HSPI
#define SD_SPI_SLOT HSPI_HOST // hspi
#elif CONFIG_SDCARD_VSPI
#define SD_SPI_SLOT VSPI_HOST // vspi
#endif

// 挂载位置
#define MOUNT_POINT "/sdcard"

#if CONFIG_IDF_TARGET_ESP32S2
#define SPI_DMA_CHAN host.slot
#elif CONFIG_IDF_TARGET_ESP32C3
#define SPI_DMA_CHAN SPI_DMA_CH_AUTO
#else
#define SPI_DMA_CHAN SPI_DMA_CH2
#endif

static TaskHandle_t xHandleSDCard = NULL; // 任务通知

/**
 * @brief SD卡是否挂载
 *
 * @return uint8_t
 */
uint8_t sdcardIsMount()
{
    if (NULL != pCardHandler) {
        return 1;
    }
    return 0;
}

#ifdef CONFIG_ESP32_SPI_SDCARD
/**
 * @brief SD卡卸载
 *
 */
static void sd_umount()
{
    if (NULL != pCardHandler) {
        xSemaphoreTake(pSPIMutex, portMAX_DELAY);
        esp_vfs_fat_sdcard_unmount(MOUNT_POINT, pCardHandler);
        spi_bus_free(SD_SPI_SLOT);
        xSemaphoreGive(pSPIMutex);
        pCardHandler = NULL;
        tips_printf("SD Card UnMount Success");
    }
}

/**
 * @brief SD卡初始化并挂载
 *
 * @return esp_err_t
 */
static esp_err_t sd_CardInit()
{
    esp_err_t ret = ESP_OK;

    sd_umount();

    // Options for mounting the filesystem.
    // If format_if_mount_failed is set to true, SD pCardHandler will be partitioned and
    // formatted in case when mounting fails.
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = SD_SPI_SLOT;
    host.max_freq_khz = 4000;

    // sdspi_slot_config_t slot_config = SDSPI_SLOT_CONFIG_DEFAULT();
    // slot_config.gpio_miso = SD_PIN_NUM_MISO;
    // slot_config.gpio_mosi = SD_PIN_NUM_MOSI;
    // slot_config.gpio_sck = SD_PIN_NUM_CLK;
    // slot_config.gpio_cs = SD_PIN_NUM_CS;
    // slot_config.dma_channel = 2;

    spi_bus_config_t bus_cfg = {
        .mosi_io_num = SD_PIN_NUM_MOSI,
        .miso_io_num = SD_PIN_NUM_MISO,
        .sclk_io_num = SD_PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000,
    };

    xSemaphoreTake(pSPIMutex, portMAX_DELAY);

    ret = spi_bus_initialize(host.slot, &bus_cfg, SPI_DMA_CHAN);
    if (ret != ESP_OK) {
        pCardHandler = NULL;
        printf("Failed to initialize bus.");
        goto error;
    }

    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = SD_PIN_NUM_CS;
    slot_config.host_id = host.slot;

    ret = esp_vfs_fat_sdspi_mount(MOUNT_POINT, &host, &slot_config, &mount_config, &pCardHandler);
    if (ret != ESP_OK) {
        pCardHandler = NULL;
        if (ret == ESP_FAIL)
            printf("Failed to mount filesystem.\r\nIf you want the pCardHandler to be formatted,\r\nset format_if_mount_failed = true.\r\n");
        else
            printf("Failed to initialize the pCardHandler (%s).\r\nMake sure SD pCardHandler lines have pull-up\r\nresistors in place.\r\n", esp_err_to_name(ret));
        goto error;
    }

    // Card has been initialized, print its properties
    sdmmc_card_print_info(stdout, pCardHandler);

error:
    xSemaphoreGive(pSPIMutex);

    return ESP_OK;
}

/**
 * @brief 获取SD卡剩余空间
 *
 * @param pFreeMb
 * @param pTotalMb
 * @return int8_t
 */
static int8_t sd_GetFree(uint32_t* pFreeMb, uint32_t* pTotalMb)
{
    if (NULL == pCardHandler)
        return -1;

    FATFS* fs;
    DWORD fre_clust, fre_sect, tot_sect;

    // Get volume information and free clusters of drive 0
    int err = f_getfree("0:", &fre_clust, &fs);
    if (err)
        return -1;

    // Get total sectors and free sectors
    tot_sect = (fs->n_fatent - 2) * fs->csize; // 总空间
    fre_sect = fre_clust * fs->csize; // 剩余空间

    // 总空间
    tot_sect /= 1024;
    tot_sect *= pCardHandler->csd.sector_size;
    tot_sect /= 1024;

    // 剩余空间
    fre_sect /= 1024;
    fre_sect *= pCardHandler->csd.sector_size;
    fre_sect /= 1024;

    *pFreeMb = fre_sect;
    *pTotalMb = tot_sect;

    return 0;
}

/**
 * @brief IO中断程序
 *
 * @param arg
 */
static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    // uint32_t gpio_num = (uint32_t)arg;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    vTaskNotifyGiveFromISR(xHandleSDCard, &xHigherPriorityTaskWoken);
}

/**
 * @brief sdcard 操作
 *
 */
static void sdcardOperate(void)
{
    int32_t level = gpio_get_level(SD_PIN_NUM_CD);

    if (0 == level && NULL == pCardHandler) {
        // 挂载SD卡
        if (sd_CardInit() == ESP_OK) {
            uint32_t FreeMb, TotalMb;
            int8_t err = sd_GetFree(&FreeMb, &TotalMb);
            if (!err) {
                printf("Available %d MB Total %d MB\r\n", FreeMb, TotalMb);
                tips_printf("SD Card Mount Success! Available %d MB", FreeMb);
            }
        }

    } else if (1 == level && NULL != pCardHandler) {
        // 卸载SD卡
        sd_umount();
    }
}

/**
 * @brief SD卡任务通知(挂载 卸载)
 *
 * @param pvParameters
 */
static void prvSDCardTask(void* pvParameters)
{
    while (true) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        vTaskDelay(1000 / portTICK_RATE_MS); // 消抖
        sdcardOperate();
    }
}

/**
 * @brief  GPIO配置为中断输入模式。并对 引脚序号、中断类型、上拉方式、下拉方式、isr中断服务函数 进行配置。
 *
 * @param  gpio_num GPIO管脚编号，0~19、21~23、25~27、32~39
 *                  （ESP32共有34个GPIO，其中 34~39 仅用作输入）
 * @param  pullup_en    上拉电阻使能与否，0/1
 * @param  pulldown_en  下拉电阻使能与否，0/1
 * @param  intr_type    中断触发类型
 *                      - GPIO_PIN_INTR_POSEDGE  上升沿中断
 *                      - GPIO_PIN_INTR_NEGEDGE  下降沿中断
 *                      - GPIO_PIN_INTR_ANYEDGE  双边沿中断
 *                      - GPIO_PIN_INTR_LOLEVEL  低电平中断
 *                      - GPIO_PIN_INTR_HILEVEL  高电平中断
 * @param  isr_handler  ISR中断处理服务函数
 *
 * @return
 *     - none
 */
static void gpiox_set_intr_input(uint32_t gpio_num, uint32_t pullup_en, uint32_t pulldown_en, uint32_t intr_type, void* isr_handler)
{
    if (-1 != gpio_num) {
        gpio_config_t io_conf;
        // IO中断类型：intr_type
        io_conf.intr_type = intr_type;
        // IO模式：输入
        io_conf.mode = GPIO_MODE_INPUT;
        // bit mask of the pins that you want to set,e.g.GPIO18/19 配置GPIO_OUT寄存器
        io_conf.pin_bit_mask = (1ULL << gpio_num);
        //下拉电阻：禁止
        io_conf.pull_down_en = pulldown_en;
        //上拉电阻：禁止
        io_conf.pull_up_en = pullup_en;
        //使用给定设置配置GPIO
        gpio_config(&io_conf);

        //安装GPIO ISR处理程序服务。为所有GPIO中断注册一个全局ISR，并通过gpio_isr_handler_add（）函数注册各个引脚处理程序。
        gpio_install_isr_service(0);
        //添加中断回调处理函数
        gpio_isr_handler_add(gpio_num, isr_handler, (void*)gpio_num);
    }
}

void sdcard_task(void* arg)
{
    gpiox_set_intr_input(SD_PIN_NUM_CD, GPIO_PULLUP_ENABLE, GPIO_PULLDOWN_DISABLE, GPIO_PIN_INTR_ANYEDGE, gpio_isr_handler);
    sdcardOperate();

    xTaskCreate(prvSDCardTask, "sdcard", 1024 * 5, NULL, tskIDLE_PRIORITY, &xHandleSDCard);
}

#endif // CONFIG_ESP32_SPI_SDCARD