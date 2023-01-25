#include "spi_lcd.h"
#include "render_task.h"

// LCD与SPI关联的句柄，通过此来调用SPI总线上的LCD设备
spi_device_handle_t LCD_SPI = NULL;

/**
 * @brief  向LCD发送1个字节的命令（D/C线电平为0）
 *      - 使用spi_device_polling_transmit，它等待直到传输完成。
 *      - 发送时同时设置D/C线为0，传输命令
 *      - 例：lcd_cmd(LCD_SPI, 0x04);
 *
 * @param  spi LCD与SPI关联的句柄，通过此来调用SPI总线上的LCD设备
 * @param  cmd 向LCD发送的1个字节命令内容
 *
 * @return
 *     - none
 */
void lcd_cmd(spi_device_handle_t spi, const uint8_t cmd)
{
    esp_err_t ret;
    spi_transaction_t t;
    memset(&t, 0, sizeof(t)); // 清空传输结构体

    t.length = 8; // SPI传输lcd命令的长度：8Bit。1个字节。（lcd的命令都是单字节的）
    t.tx_buffer = &cmd; // 数据是cmd本身
    t.user = (void*)0; // D/C 线电平为0，传输命令

    // ret = spi_device_polling_transmit(spi, &t); // 开始传输
    xSemaphoreTake(pSPIMutex, portMAX_DELAY);
    ret = spi_device_transmit(spi, &t); // Transmit!
    xSemaphoreGive(pSPIMutex);
    assert(ret == ESP_OK); // 应该没有问题
}

/**
 * @brief  向LCD发送长度为len个字节的数据（D/C线电平为1）
 *      - 发送时同时设置D/C线为1，传输数据
 *      - 例：lcd_data(LCD_SPI, dataBuf, 10);
 *
 * @param  spi LCD与SPI关联的句柄，通过此来调用SPI总线上的LCD设备
 * @param  data 要发送数据的指针
 * @param  len 发送的字节数
 *
 * @return
 *     - none
 */
void lcd_data(spi_device_handle_t spi, const uint8_t* data, int len)
{
    if (len == 0)
        return; // len为0直接返回，无需发送任何东西

    esp_err_t ret;
    spi_transaction_t t;
    memset(&t, 0, sizeof(t)); // 清空传输结构体

    t.length = len * 8; // len单位为字节, 发送长度的单位是Bit
    t.tx_buffer = data; // 数据指针
    t.user = (void*)1; // D/C 线电平为1，传输数据

    // ret = spi_device_polling_transmit(spi, &t); // 开始传输
    xSemaphoreTake(pSPIMutex, portMAX_DELAY);
    ret = spi_device_transmit(spi, &t); // Transmit!
    xSemaphoreGive(pSPIMutex);
    assert(ret == ESP_OK); // 应该没有问题
}

/**
 * @brief  向LCD发送单点16Bit的像素数据，（根据驱动IC的不同，可能为2或3个字节，需要转换RGB565、RGB666）
 *      - ili9488\ili9481 这类IC，SPI总线仅能使用RGB666-18Bit/像素，分3字节传输。而不能使用16Bit/像素，分2字节传输。（0x3A寄存器）
 *      - 例：lcd_data16(LCD_SPI, RED);
 *
 * @param  spi LCD与SPI关联的句柄，通过此来调用SPI总线上的LCD设备
 * @param  data 要发送的单点像素数据，uint16_t的RGB565
 *
 * @return
 *     - none
 */
void lcd_data16(spi_device_handle_t spi, uint16_t data)
{
#if !(CONFIG_LCD_TYPE_ILI9488 || CONFIG_LCD_TYPE_ILI9481)
    // 16Bit/像素，2字节/像素
    esp_err_t ret;
    spi_transaction_t t;
    uint8_t dataBuf[2] = { 0 };
    dataBuf[0] = data >> 8;
    dataBuf[1] = data & 0xFF;
    memset(&t, 0, sizeof(t)); // 清空传输结构体
    t.length = 2 * 8; // SPI传输数据的长度：16Bit。2个字节
    t.tx_buffer = dataBuf; // 数据指针
    t.user = (void*)1; // D/C 线电平为1，传输数据
    ret = spi_device_polling_transmit(spi, &t); // 开始传输
    assert(ret == ESP_OK); // 应该没有问题
#else
    // ili9488用SPI总线驱动，像素必须为18Bit/pixel，3字节/像素。16Bit会没有显示。（需要提前设置0x3A寄存器）
    esp_err_t ret;
    spi_transaction_t t;
    uint8_t dataBuf[3] = { 0 };
    // 将原本2个字节的RGB565，转换为3个字节的RGB666
    dataBuf[0] = (data >> 8) & 0xF8; // RED
    dataBuf[1] = (data >> 3) & 0xFC; // GREEN
    dataBuf[2] = data << 3; // BLUE
    memset(&t, 0, sizeof(t)); // 清空传输结构体
    t.length = 3 * 8; // SPI传输数据的长度：24Bit。3个字节
    t.tx_buffer = dataBuf; // 数据指针
    t.user = (void*)1; // D/C 线电平为1，传输数据
    ret = spi_device_polling_transmit(spi, &t); // 开始传输
    assert(ret == ESP_OK); // 应该没有问题*/
#endif
}

/**
 * @brief  获取LCD的ID
 *      - 由于通常MISO引脚都不接，而且各驱动IC的寄存器定义有差异，导致大多数情况都得不到LCD的ID
 *      - 例：lcd_get_id(LCD_SPI);
 *
 * @param  spi LCD与SPI关联的句柄，通过此来调用SPI总线上的LCD设备
 *
 * @return
 *     - none
 */
#if 0
static uint32_t lcd_get_id(spi_device_handle_t spi)
{
    // get_id cmd
    lcd_cmd(spi, 0x04);

    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length = 8 * 3;
    t.flags = SPI_TRANS_USE_RXDATA;
    t.user = (void*)1;

    esp_err_t ret = spi_device_polling_transmit(spi, &t);
    assert(ret == ESP_OK);

    return *(uint32_t*)t.rx_data;
}
#endif

/**
 * @brief  初始化LCD的驱动IC
 *      - 根据 spi_lcd.h 中对IC型号的定义，将预先准备好的参数写入到LCD驱动IC内
 *      - lcd_ic_init(LCD_SPI);
 *
 * @param  spi LCD与SPI关联的句柄，通过此来调用SPI总线上的LCD设备
 *
 * @return
 *     - none
 */
static void lcd_ic_init(spi_device_handle_t spi)
{
    int cmd = 0;
    const lcd_init_cmd_t* lcd_init_cmds;

    // 初始化 DC、RST、BLK 引脚，配置为推挽输出
    gpio_set_direction(SPI_LCD_PIN_NUM_DC, GPIO_MODE_OUTPUT); // 数据 / 命令
    gpio_set_direction(SPI_LCD_PIN_NUM_RST, GPIO_MODE_OUTPUT); // 复位

    // RST引脚拉低，复位后重新拉高
    gpio_set_level(SPI_LCD_PIN_NUM_RST, 0);
    vTaskDelay(100 / portTICK_RATE_MS);

    gpio_set_level(SPI_LCD_PIN_NUM_RST, 1);
    vTaskDelay(200 / portTICK_RATE_MS);

    // uint32_t lcd_id = lcd_get_id(spi);
    // printf("LCD ID = %x.\n", lcd_id);

#ifdef CONFIG_LCD_TYPE_AUTO

#elif defined(CONFIG_LCD_TYPE_ST7735)
    printf("LCD ST7735 initialization.\n");
    lcd_init_cmds = st_7735_init_cmds; // st_init_cmds;
#elif defined(CONFIG_LCD_TYPE_ST7735S)
    printf("LCD ST7735 initialization.\n");
    lcd_init_cmds = st_7735S_init_cmds; // st_init_cmds;
#elif defined(CONFIG_LCD_TYPE_ST7789V)
    printf("LCD ST7789V initialization.\n");
    lcd_init_cmds = st_7789V_init_cmds;
#elif defined(CONFIG_LCD_TYPE_ILI9341)
    printf("LCD ILI9341 initialization.\n");
    lcd_init_cmds = ili_9341_init_cmds;
#elif defined(CONFIG_LCD_TYPE_ILI9488)
    printf("LCD ILI9488 initialization.\n");
    lcd_init_cmds = ili_9488_init_cmds;
#elif defined(CONFIG_LCD_TYPE_ILI9481)
    printf("LCD ILI9481 initialization.\n");
    lcd_init_cmds = ili_9481_init_cmds;
#elif defined(CONFIG_LCD_TYPE_HX8357C)
    printf("LCD HX8357C initialization.\n");
    lcd_init_cmds = hx_8357c_init_cmds;
#endif

    // 将spi_lcd.h 中的参数写入LCD驱动IC
    while (lcd_init_cmds[cmd].databytes != 0xff) {
        lcd_cmd(spi, lcd_init_cmds[cmd].cmd);
        lcd_data(spi, lcd_init_cmds[cmd].data, lcd_init_cmds[cmd].databytes & 0x1F);
        if (lcd_init_cmds[cmd].databytes & 0x80) {
            vTaskDelay(10 / portTICK_RATE_MS);
        }
        cmd++;
    }
}

/**
 * @brief 设置发送数据 还是 命令
 * SPI预传输回调。（在SPI传输即将开始前调用的函数）用来处理 D/C 线的电平，来控制接下来发送的内容是 指令/数据
 * This function is called (in irq context!) just before a transmission starts. It will set the D/C line to the value indicated in the user field.
 * @param t
 */
static void lcd_spi_pre_transfer_callback(spi_transaction_t* t)
{
    int dc = (int)t->user;
    gpio_set_level(SPI_LCD_PIN_NUM_DC, dc);
}

/**
 * @brief  以SPI方式驱动LCD初始化函数
 *      - 过程包括：关联 SPI总线及LCD设备、驱动IC的参数配置、点亮背光、设置LCD的安装方向、设置屏幕分辨率、扫描方向、初始化显示区域的大小
 *      - （注意：普通GPIO最大只能30MHz，而IOMUX默认的SPI，CLK最大可以设置到80MHz）
 *      - 例：spi_lcd_init(SPI2_HOST, 60*1000*1000, SPI_LCD_PIN_NUM_CS);
 *
 * @param  host_id SPI端口号。SPI1_HOST / SPI2_HOST / SPI3_HOST
 * @param  clk_speed LCD设备的SPI速度（注意：普通GPIO最大只能30MHz，而IOMUX默认的SPI，CLK最大可以设置到80MHz）
 * @param  cs_io_num CS端口号，尽量使用IOMUX默认的IO
 *
 * @return
 *     - none
 */
void spi_lcd_init(spi_host_device_t host_id, uint32_t clk_speed, gpio_num_t cs_io_num)
{
    esp_err_t ret;

    // LCD设备初始化
    // 先关联 SPI总线及LCD设备
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = clk_speed, // CLK时钟频率
        .mode = 0, // SPI mode 0
        .spics_io_num = cs_io_num, // CS引脚定义
        .queue_size = 7, // 事务队列大小为7
        .pre_cb = lcd_spi_pre_transfer_callback, // 指定预传输回调，以处理 D/C线电平，来区别发送命令/数据
    };

    // 将LCD外设与SPI总线关联
    ret = spi_bus_add_device(host_id, &devcfg, &LCD_SPI);
    ESP_ERROR_CHECK(ret);

    // LCD 驱动IC的参数配置（自动识别始终不成功、需要到.h文件中手动配置）
    lcd_ic_init(LCD_SPI);
}

/**
 * @brief  配置SPIx主机模式，配置DMA通道、DMA字节大小，及 MISO、MOSI、CLK的引脚。
 *      - （注意：普通GPIO最大只能30MHz，而IOMUX默认的SPI-IO，CLK最大可以设置到80MHz）
 *      - 例：spi_master_init(SPI2_HOST, LCD_DEF_DMA_CHAN, LCD_DMA_MAX_SIZE, SPI_LCD_PIN_NUM_MISO, SPI_LCD_PIN_NUM_MOSI, SPI_LCD_PIN_NUM_CLK);
 *
 * @param  host_id SPI端口号。SPI1_HOST / SPI2_HOST / SPI3_HOST
 * @param  dma_chan 使用的DMA通道
 * @param  max_tran_size DMA最大的传输字节数（会根据此值给DMA分配内存，值越大分配给DMA的内存就越大，单次可用DMA传输的内容就越多）
 * @param  miso_io_num MISO端口号。除仅能做输入 和 6、7、8、9、10、11之外的任意端口，但仅IOMUX默认的SPI-IO才能达到最高80MHz上限。
 * @param  mosi_io_num MOSI端口号
 * @param  clk_io_num CLK端口号
 *
 * @return
 *     - none
 */
void spi_master_init(spi_host_device_t host_id, int dma_chan, uint32_t max_tran_size, gpio_num_t miso_io_num, gpio_num_t mosi_io_num, gpio_num_t clk_io_num)
{
    esp_err_t ret;

    // 配置 MISO、MOSI、CLK、CS 的引脚，和DMA最大传输字节数
    spi_bus_config_t buscfg = {
        .miso_io_num = miso_io_num, // MISO引脚定义
        .mosi_io_num = mosi_io_num, // MOSI引脚定义
        .sclk_io_num = clk_io_num, // CLK引脚定义
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = max_tran_size, // 最大传输字节数
    };

    // 初始化SPI总线
    ret = spi_bus_initialize(host_id, &buscfg, dma_chan);
    ESP_ERROR_CHECK(ret);
}

/*//////////////////////////////////////////////////// 硬件SPI底层总线适配层 ////////////////////////////////////////////////////*/

/**
 * @brief SPI操作LCD寄存器函数，写命令（固定长度，LCD的寄存器地址都是单字节-8Bits）
 *        原型为 "spi_lcd.c" 中 void lcd_cmd(spi_device_handle_t spi, const uint8_t cmd)
 *
 * @param reg_addr
 */
void LCD_WR_REG(uint8_t reg_addr)
{
    lcd_cmd(LCD_SPI, reg_addr);
}

/**
 * @brief SPI写LCD数据函数，写数据（可变长度）
 *        原型为 "spi_lcd.c" 中 void lcd_data(spi_device_handle_t spi, const uint8_t *data, int len)
 *
 * @param data
 * @param len
 */
void LCD_WR_DATA(const uint8_t* data, int len)
{
    lcd_data(LCD_SPI, data, len);
}

/**
 * @brief SPI向LCD发送单点16Bit的像素数据（像素长度固定，但根据驱动IC的不同可能是RGB565分16Bit-2字节，也可能为RGB666-18Bit分3字节传输）
 *        原型为 "spi_lcd.c" 中 void lcd_data16(spi_device_handle_t spi, uint16_t data)
 *
 * @param data
 */
void LCD_WR_DATA16(uint16_t data)
{
    lcd_data16(LCD_SPI, data);
}

/**
 * @brief 写LCD寄存器，写单字节命令
 *
 * @param LCD_Reg
 * @param LCD_RegValue
 */
void LCD_WriteReg(uint8_t LCD_Reg, uint8_t LCD_RegValue)
{
    LCD_WR_REG(LCD_Reg);
    LCD_WR_DATA(&LCD_RegValue, 1);
}

/**
 * @brief 开始写GRAM，命令（0x2C）
 *
 */
void LCD_WriteRAM_Prepare(void)
{
    LCD_WR_REG(0x2C);
}

// 推荐一个很全的LCD驱动库，arduino 的TFT_eSPI库：https://github.com/Bodmer/TFT_eSPI/blob/master/TFT_Drivers/ILI9341_Init.h
// 还是要以买到的LCD的卖家资料为首要参考，有时相同IC但不同厂家的LCD的驱动参数都会不同。
