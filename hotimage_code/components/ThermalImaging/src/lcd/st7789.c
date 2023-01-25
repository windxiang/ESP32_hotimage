#include "thermalimaging.h"
#include <driver/gpio.h>
#include <driver/spi_master.h>
#include <esp_system.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <soc/gpio_struct.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef CONFIG_ESP32_SPI_ST7789_LCD
#if (ST7789_MODE == ST7789_BUFFER_MODE)
// LCD 缓存
/* EXT_RAM_ATTR */ static uint16_t ScreenBuff[LINE_PIXEL_MAX_SIZE * ROW_PIXEL_MAX_SIZE]; // LCD显存
#endif

#if (ST7789_MODE == ST7789_DIRECT_MODE)
static uint16_t* ScreenBuff = NULL;
#endif
#endif // CONFIG_ESP32_SPI_ST7789_LCD

_lcd_dev lcddev;

/**
 * @brief  设置光标位置（写入窗口大小为 当前光标~全屏）
 * 		- 若驱动IC为 stxxxx，需要到此函数修改GRAM偏移基地址。ili没有偏移地址。
 *
 * @param  Xpos 横坐标（像素数）
 * @param  Ypos 纵坐标（像素数）
 *
 * @return
 *     - none
 */
void LCD_SetCursor(uint16_t Xpos, uint16_t Ypos)
{
#ifdef CONFIG_ESP32_SPI_ST7789_LCD
    uint8_t databuf[4] = { 0 };
    uint8_t xBase = 0, yBase = 0;

#ifdef CONFIG_LCD_TYPE_ST7735
    // 如果驱动IC为 ST7735，则需要增加偏移基地址。横屏：x-3，y-2
    xBase = 3;
    yBase = 2;
#elif defined(CONFIG_LCD_TYPE_ST7735S)
    // 如果驱动IC为 ST7735，则需要增加偏移基地址。横屏：x-0，y-24
    xBase = 0;
    yBase = 24;
#endif
    // ili系列没有偏移基地址
    databuf[0] = (Xpos + xBase) >> 8;
    databuf[1] = (Xpos + xBase) & 0XFF;
    databuf[2] = (lcddev.width - 1 + xBase) >> 8;
    databuf[3] = (lcddev.width - 1 + xBase) & 0XFF;

    LCD_WR_REG(lcddev.setxcmd);
    LCD_WR_DATA(databuf, 4);

    databuf[0] = (Ypos + yBase) >> 8;
    databuf[1] = (Ypos + yBase) & 0XFF;
    databuf[2] = (lcddev.height - 1 + yBase) >> 8;
    databuf[3] = (lcddev.height - 1 + yBase) & 0XFF;

    LCD_WR_REG(lcddev.setycmd);
    LCD_WR_DATA(databuf, 4);
#endif // CONFIG_ESP32_SPI_ST7789_LCD
}

/**
 * @brief  设置写入窗口大小，并设置写入光标的位置
 * 		- 区别于上一句 LCD_SetCursor。LCD_SetWindows更为灵活。（这种方式在需要局部刷新，例如操作字符取模的写入上更加高效）
 * 		- 若驱动IC为 stxxxx，需要到此函数修改GRAM偏移基地址。ili没有偏移地址。
 *
 * @param  xStar/xEnd 设置列的起始和结束地址
 * @param  yStar/yEnd 设置行的起始和结束地址
 *
 * @return
 *     - none
 */
void LCD_SetWindows(uint16_t xStar, uint16_t yStar, uint16_t xEnd, uint16_t yEnd)
{
#ifdef CONFIG_ESP32_SPI_ST7789_LCD
    uint8_t databuf[4] = { 0 };
#ifdef CONFIG_LCD_TYPE_ST7735
    // 如果驱动IC为 ST7735，则需要增加偏移基地址。横屏：x-3，y-2
    xStar += 3; // 加偏移基地址
    xEnd += 3; // 加偏移基地址
    yStar += 2; // 加偏移基地址
    yEnd += 2; // 加偏移基地址
#elif defined(CONFIG_LCD_TYPE_ST7735S)
    // 如果驱动IC为 ST7735S，则需要增加偏移基地址。横屏：x-0，y-24
    xStar += 0; // 加偏移基地址
    xEnd += 0; // 加偏移基地址
    yStar += 24; // 加偏移基地址
    yEnd += 24; // 加偏移基地址
#endif
    // ili系列没有偏移基地址
    databuf[0] = xStar >> 8;
    databuf[1] = 0xFF & xStar;
    databuf[2] = xEnd >> 8;
    databuf[3] = 0xFF & xEnd;
    LCD_WR_REG(0x2A);
    LCD_WR_DATA(databuf, 4);

    databuf[0] = yStar >> 8;
    databuf[1] = 0xFF & yStar;
    databuf[2] = yEnd >> 8;
    databuf[3] = 0xFF & yEnd;
    LCD_WR_REG(0x2B);
    LCD_WR_DATA(databuf, 4);

    LCD_WR_REG(0x2C); //开始写入GRAM
#endif // CONFIG_ESP32_SPI_ST7789_LCD
}

/**
 * @brief  设置LCD的自动扫描方向（寄存器地址0x36）
 * 		- 修改LCD扫描方向的目的是，以适配屏幕的不同安装方向。
 * 		- 若驱动IC为 stxxxx，修改扫描方向后会影响到 LCD_SetCursor、LCD_SetWindows 中的GRAM偏移基地址
 *
 * @param  dir LCD的安装方向。L2R_U2D/L2R_D2U/R2L_U2D/R2L_D2U/U2D_L2R/U2D_R2L/D2U_L2R/D2U_R2L,代表8个方向(包括左右镜像翻转)
 *
 * @return
 *     - none
 */
void LCD_Scan_Dir(uint8_t dir)
{
#ifdef CONFIG_ESP32_SPI_ST7789_LCD
    uint8_t regval = 0;
    uint8_t dirreg = 0;

    // 0x36寄存器，设置扫描方向和方式
    switch (dir) {
    case L2R_U2D: //从左到右,从上到下
        regval |= (0 << 7) | (0 << 6) | (0 << 5);
        break;

    case L2R_D2U: //从左到右,从下到上
        regval |= (1 << 7) | (0 << 6) | (0 << 5);
        break;

    case R2L_U2D: //从右到左,从上到下
        regval |= (0 << 7) | (1 << 6) | (0 << 5);
        break;

    case R2L_D2U: //从右到左,从下到上
        regval |= (1 << 7) | (1 << 6) | (0 << 5);
        break;

    case U2D_L2R: //从上到下,从左到右
        regval |= (0 << 7) | (0 << 6) | (1 << 5);
        break;

    case U2D_R2L: //从上到下,从右到左
        regval |= (0 << 7) | (1 << 6) | (1 << 5);
        break;

    case D2U_L2R: //从下到上,从左到右
        regval |= (1 << 7) | (0 << 6) | (1 << 5);
        break;

    case D2U_R2L: //从下到上,从右到左
        regval |= (1 << 7) | (1 << 6) | (1 << 5);
        break;
    }

    dirreg = 0X36;

#if CONFIG_LCD_TYPE_ST7789V
    regval |= 0X00; // RGB
    // 翻转Bit7
    if (regval & 0x80) {
        // 如果Bit7=1，清0
        regval &= 0x7F;
    } else {
        // 如果Bit7=0，置1
        regval |= 0x80;
    }

#elif (CONFIG_LCD_TYPE_ST7735 || CONFIG_LCD_TYPE_ST7735S)
    regval |= 0X08; // BGR
    // 翻转Bit7
    if (regval & 0x80) {
        // 如果Bit7=1，清0
        regval &= 0x7F;
    } else {
        // 如果Bit7=0，置1
        regval |= 0x80;
    }

#else
    // ili系列屏幕
    regval |= 0X08; // BGR
#endif

    LCD_WriteReg(dirreg, regval);

    if ((regval & 0X20) || lcddev.dir == 1) {
        // 交换X,Y
        if (lcddev.width < lcddev.height) {
            uint16_t temp = lcddev.width;
            lcddev.width = lcddev.height;
            lcddev.height = temp;
        }

    } else {
        // 交换X,Y
        if (lcddev.width > lcddev.height) {
            uint16_t temp = lcddev.width;
            lcddev.width = lcddev.height;
            lcddev.height = temp;
        }
    }

    LCD_SetWindows(0, 0, (lcddev.width - 1), (lcddev.height - 1));
#endif // CONFIG_ESP32_SPI_ST7789_LCD
}

/**
 * @brief  设置LCD的横竖屏方向，并根据型号匹配像素数目（显示范围）
 * 		- 修改LCD扫描方向的目的是，以适配屏幕的不同安装方向。
 * 		- 若驱动IC为 stxxxx，修改扫描方向后会影响到 LCD_SetCursor、LCD_SetWindows 中的GRAM偏移基地址
 *
 * @param  dir LCD的安装方向是竖屏还是横屏。vertical / horizontal
 *
 * @return
 *     - none
 */
static void LCD_Display_Resolution(lcd_display_dir_t dir)
{
    // 设置写GRAM命令、设置X/Y坐标命令
    // 并根据LCD的安装方向，来定义GUI的显示范围（分辨率）
    if (dir == vertical) {
        // 竖屏
        lcddev.dir = 0;
        lcddev.wramcmd = 0x2C;
        lcddev.setxcmd = 0x2A;
        lcddev.setycmd = 0x2B;

        // 根据屏幕硬件像素数目，及安装方向，来定义GUI的显示范围（分辨率）
#ifdef LCD_PIXEL_SIZE_80_160
        lcddev.width = 80;
        lcddev.height = 160;
#elif defined(LCD_PIXEL_SIZE_128_128)
        lcddev.width = 128;
        lcddev.height = 128;
#elif defined(LCD_PIXEL_SIZE_135_240)
        lcddev.width = 135;
        lcddev.height = 240;
#elif defined(LCD_PIXEL_SIZE_240_240)
        lcddev.width = 240;
        lcddev.height = 240;
#elif defined(LCD_PIXEL_SIZE_240_320)
        lcddev.width = 240;
        lcddev.height = 320;
#elif defined(LCD_PIXEL_SIZE_320_480)
        lcddev.width = 320;
        lcddev.height = 480;
#endif

    } else {
        //横屏
        lcddev.dir = 1;
        lcddev.wramcmd = 0x2C;
        lcddev.setxcmd = 0x2A;
        lcddev.setycmd = 0x2B;

        // 根据屏幕硬件像素数目，及安装方向，来定义GUI的显示范围（分辨率）
#ifdef LCD_PIXEL_SIZE_80_160
        lcddev.width = 160;
        lcddev.height = 80;
#elif defined(LCD_PIXEL_SIZE_128_128)
        lcddev.width = 128;
        lcddev.height = 128;
#elif defined(LCD_PIXEL_SIZE_135_240)
        lcddev.width = 240;
        lcddev.height = 135;
#elif defined(LCD_PIXEL_SIZE_240_240)
        lcddev.width = 240;
        lcddev.height = 240;
#elif defined(LCD_PIXEL_SIZE_240_320)
        lcddev.width = 320;
        lcddev.height = 240;
#elif defined(LCD_PIXEL_SIZE_320_480)
        lcddev.width = 480;
        lcddev.height = 320;
#endif
    }
}

/**
 * @brief  设置LCD的显示方向
 * 		- 根据 横竖屏、是否倒置、是否镜像 去匹配LCD的扫描方向
 * 		- 若驱动IC为 stxxxx，修改扫描方向后会影响到 LCD_SetCursor、LCD_SetWindows 中的GRAM偏移基地址
 *
 * @param  dir LCD的安装方向是竖屏还是横屏。vertical / horizontal
 * @param  invert LCD是否倒置。invert_dis / invert_en
 * @param  mirror LCD是否镜像安装。mirror_dis / mirror_en（可用于镜面反射及棱镜的镜像显示，可使画面左右翻转。参照分光棱镜）
 *
 * @return
 *     - none
 */
static void LCD_Display_Dir(lcd_display_dir_t dir, lcd_display_invert_t invert, lcd_display_mirror_t mirror)
{
    lcd_scan_type_t scan_type = 0;

    // 设置屏幕扫描方向，来匹配屏幕的安装方向。或镜像安装方式（可用于镜面反射及棱镜的镜像显示，例如分光棱镜及镜面反射显示）
    if ((dir == vertical) && (invert == invert_dis) && (mirror == mirror_dis)) {
        // 竖屏、不翻转(正着摆放)、不镜像
        // 竖屏，从左到右，从上到下
        scan_type = R2L_U2D;
    } else if ((dir == vertical) && (invert == invert_dis) && (mirror == mirror_en)) {
        // 竖屏、不翻转(正着摆放)、镜像
        // 竖屏，从右到左，从上到下
        scan_type = L2R_U2D;
    } else if ((dir == vertical) && (invert == invert_en) && (mirror == mirror_dis)) {
        // 竖屏、翻转(反着摆放)、不镜像
        // 竖屏，从左到右，从下到上
        scan_type = L2R_D2U;
    } else if ((dir == vertical) && (invert == invert_en) && (mirror == mirror_en)) {
        // 竖屏、翻转(反着摆放)、镜像
        // 竖屏，从右到左，从下到上
        scan_type = R2L_D2U;
    } else if ((dir == horizontal) && (invert == invert_dis) && (mirror == mirror_dis)) {
        // 横屏、不翻转(正着摆放)、不镜像
        // 横屏，从上到下，从左到右
        scan_type = U2D_L2R;
    } else if ((dir == horizontal) && (invert == invert_dis) && (mirror == mirror_en)) {
        // 横屏、不翻转(正着摆放)、镜像
        // 横屏，从上到下，从右到左
        scan_type = D2U_L2R;
    } else if ((dir == horizontal) && (invert == invert_en) && (mirror == mirror_dis)) {
        // 横屏、翻转(反着摆放)、不镜像
        // 横屏，从下到上，从左到右
        scan_type = D2U_R2L;
    } else if ((dir == horizontal) && (invert == invert_en) && (mirror == mirror_en)) {
        // 横屏、翻转(反着摆放)、镜像
        // 横屏，从下到上，从右到左
        scan_type = U2D_R2L;
    }
    LCD_Scan_Dir(scan_type); // 根据屏幕的不同安装方式，来适配不同的扫描方向
}

/**
 * @brief 设置绘图矩形的4个点坐标
 *
 * @param x 起始坐标
 * @param y 起始坐标
 * @param x_end 终点坐标
 * @param y_end 终点坐标
 */
void st7789_setWindow(uint16_t x, uint16_t y, uint16_t x_end, uint16_t y_end)
{
#ifdef CONFIG_ESP32_SPI_ST7789_LCD
    uint8_t Buff[4];

    lcd_cmd(LCD_SPI, ST7789_CASET); // Column addr set
    Buff[0] = (x >> 8) & 0xFF;
    Buff[1] = x & 0xFF;
    Buff[2] = ((x + x_end - 1) >> 8) & 0xFF;
    Buff[3] = (x + x_end - 1) & 0xFF;
    lcd_data(LCD_SPI, Buff, 4);

    lcd_cmd(LCD_SPI, ST7789_RASET); // Row addr set
    Buff[0] = (y >> 8) & 0xFF;
    Buff[1] = y & 0xFF;
    Buff[2] = ((y + y_end - 1) >> 8) & 0xFF;
    Buff[3] = (y + y_end - 1) & 0xFF;
    lcd_data(LCD_SPI, Buff, 4);

    lcd_cmd(LCD_SPI, ST7789_RAMWR); // write to RAM
#endif // CONFIG_ESP32_SPI_ST7789_LCD
}

/**
 * @brief 关闭显示
 *
 */
void st7789_DisplayOff(void)
{
#ifdef CONFIG_ESP32_SPI_ST7789_LCD
    lcd_cmd(LCD_SPI, ST7789_DISPOFF);
#endif // CONFIG_ESP32_SPI_ST7789_LCD
}

#ifdef CONFIG_ESP32_SPI_ST7789_LCD
/**
 * @brief 交换颜色
 *
 * @param color
 */
static void SwapBytes(uint16_t* color)
{
    uint8_t temp = *color >> 8;
    *color = (*color << 8) | temp;
}
#endif // CONFIG_ESP32_SPI_ST7789_LCD

#if (ST7789_MODE == ST7789_DIRECT_MODE)
//==============================================================================
// 直接显示模式
//==============================================================================
/**
 * @brief 绘制一个像素
 *
 * @param x 起始横坐标
 * @param y 起始纵坐标
 * @param color 颜色
 */
void st7789_DrawPixel(int16_t x, int16_t y, uint16_t color)
{
#ifdef CONFIG_ESP32_SPI_ST7789_LCD
    if ((x < 0) || (x >= lcddev.width) || (y < 0) || (y >= lcddev.height))
        return;

    SwapBytes(&color);

    st7789_setWindow(x, y, x, y);
    lcd_data(LCD_SPI, (uint8_t*)&color, 2);
#endif // CONFIG_ESP32_SPI_ST7789_LCD
}

/**
 * @brief 填充矩形
 *
 * @param x 起始横坐标
 * @param y 起始纵坐标
 * @param w 宽
 * @param h 高
 * @param color 颜色
 */
void st7789_FillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
{
#ifdef CONFIG_ESP32_SPI_ST7789_LCD
    if ((w <= 0) || (h <= 0) || (x >= lcddev.width) || (y >= lcddev.height))
        return;

    if (x < 0) {
        x = 0;
    }
    if (y < 0) {
        y = 0;
    }

    if ((x + w) > lcddev.width)
        w = lcddev.width - x;

    if ((y + h) > lcddev.height)
        h = lcddev.height - y;

    uint32_t lineSize = w * 2;
    uint16_t lineBuf[LINE_PIXEL_MAX_SIZE]; // 一行LCD显存

    SwapBytes(&color);

    for (uint16_t i = 0; i < w; i++) {
        lineBuf[i] = color;
    }

    st7789_setWindow(x, y, w, h);

    for (uint16_t i = 0; i < h; i++) {
        lcd_data(LCD_SPI, (uint8_t*)lineBuf, lineSize);
    }
#endif
}
#endif // ST7789_MODE == ST7789_DIRECT_MODE

#if (ST7789_MODE == ST7789_BUFFER_MODE)
//==============================================================================
// 显存模式
//==============================================================================

/**
 * @brief 绘制一个像素
 *
 * @param x 起始横坐标
 * @param y 起始纵坐标
 * @param color 颜色
 */
void st7789_DrawPixel(int16_t x, int16_t y, uint16_t color)
{
#ifdef CONFIG_ESP32_SPI_ST7789_LCD
    if ((x < 0) || (x >= lcddev.width) || (y < 0) || (y >= lcddev.height))
        return;

    SwapBytes(&color);

    ScreenBuff[y * lcddev.width + x] = color;
#endif // CONFIG_ESP32_SPI_ST7789_LCD
}

/**
 * @brief 填充矩形
 *
 * @param x 起始横坐标
 * @param y 起始纵坐标
 * @param w 宽
 * @param h 高
 * @param color 颜色
 */
void st7789_FillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
{
#ifdef CONFIG_ESP32_SPI_ST7789_LCD
    if ((w <= 0) || (h <= 0) || (x >= lcddev.width) || (y >= lcddev.height))
        return;

    if (x < 0) {
        x = 0;
    }
    if (y < 0) {
        y = 0;
    }

    if ((x + w) > lcddev.width)
        w = lcddev.width - x;

    if ((y + h) > lcddev.height)
        h = lcddev.height - y;

    SwapBytes(&color);

    for (uint16_t row = 0; row < h; row++) {
        for (uint16_t col = 0; col < w; col++) {
            ScreenBuff[(y + row) * lcddev.width + x + col] = color;
        }
    }
#endif // CONFIG_ESP32_SPI_ST7789_LCD
}

/**
 * @brief 刷新一帧
 *
 */
void st7789_update(void)
{
#ifdef CONFIG_ESP32_SPI_ST7789_LCD
    st7789_setWindow(0, 0, lcddev.width, lcddev.height);
    lcd_data(LCD_SPI, (uint8_t*)ScreenBuff, sizeof(ScreenBuff));
#endif // CONFIG_ESP32_SPI_ST7789_LCD
}

/**
 * @brief 复制显存数据到指定内存,保存位图的时候使用到
 *
 * @param pBuff 目标内存
 */
void st7789_getScreenData(uint16_t* pBuff)
{
#ifdef CONFIG_ESP32_SPI_ST7789_LCD
    for (uint32_t pixel = 0; pixel < lcddev.height * lcddev.width; pixel++, pBuff++)
        *pBuff = (ScreenBuff[pixel] >> 8) | (ScreenBuff[pixel] << 8);
#endif // CONFIG_ESP32_SPI_ST7789_LCD
}

/**
 * @brief 获取指定位置的颜色
 *
 * @param x 指定位置横坐标
 * @param y 指定位置纵坐标
 * @return uint16_t 返回指定位置的颜色
 */
uint16_t st7789_GetPixel(int16_t x, int16_t y)
{
#ifdef CONFIG_ESP32_SPI_ST7789_LCD
    if ((x < 0) || (x >= lcddev.width) || (y < 0) || (y >= lcddev.height))
        return 0;

    uint16_t color = ScreenBuff[y * lcddev.width + x];
    SwapBytes(&color);
    return color;
#else
    return 0x0000;
#endif // CONFIG_ESP32_SPI_ST7789_LCD
}
#endif //  ST7789_MODE == ST7789_BUFFER_MODE

/**
 * @brief 初始化ST7789液晶
 *
 */
void st7789_init()
{
#if (ST7789_MODE == ST7789_DIRECT_MODE)
    ScreenBuff = heap_caps_malloc((LINE_PIXEL_MAX_SIZE * ROW_PIXEL_MAX_SIZE) << 1, MALLOC_CAP_8BIT);
#endif

    // 配置SPI3-主机模式，配置DMA通道、DMA字节大小，及 MISO、MOSI、CLK的引脚。
    spi_master_init(LCD_SPI_SLOT, LCD_DEF_DMA_CHAN, LCD_DMA_MAX_SIZE, SPI_LCD_PIN_NUM_MISO, SPI_LCD_PIN_NUM_MOSI, SPI_LCD_PIN_NUM_CLK);

    // lcd-驱动IC初始化（注意：普通GPIO最大只能30MHz，而IOMUX默认的SPI引脚，CLK最大可以设置到80MHz）（注意排线不要太长，高速时可能会花屏）
    spi_lcd_init(LCD_SPI_SLOT, /* 80 * 1000 * 1000 */ CONFIG_LCD_SPI_CLOCK, SPI_LCD_PIN_NUM_CS);

    // 设置LCD的安装方向、及横竖方向的像素数目（需要与下面的扫描方式保持一致）
    LCD_Display_Resolution(LCD_DIR);

    // 设置屏幕分辨率、扫描方向
    // 初始化 显示区域的大小，和扫描方向。
    // 来匹配屏幕的安装方向。或镜像安装方式（可用于镜面反射及棱镜的镜像显示）（提供了8中扫描方式，以便横竖屏、翻转和镜像的切换）
    LCD_Display_Dir(LCD_DIR, LCD_INVERT, LCD_MIRROR);

    // 清空成黑色
    st7789_FillRect(0, 0, lcddev.width, lcddev.height, BLACK);
    st7789_update();

    // 初始化背光PWM
    lcd_backlight_init();
}
