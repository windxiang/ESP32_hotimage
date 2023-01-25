#ifndef __SPI_LCD_H__
#define __SPI_LCD_H__

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "freertos/task.h"
#include <string.h>

/**
 * @brief 大多数屏幕的安装方向定义（以手册为准）
 *
 * @param 横屏 正向摆放
 *
    +----------------------------+
    |                           ||
    |                           ||--------
    |                           ||-- 排 --
    |                           ||-- 线 --
    |                           ||--------
    |                           ||
    +----------------------------+

 * @param 横屏 倒立摆放
 *
            +----------------------------+
            ||                           |
    --------||                           |
    -- 排 --||                           |
    -- 线 --||                           |
    --------||                           |
            ||                           |
            +----------------------------+


 * @param 竖屏 正向摆放
 *
    +----------------+
    |                |
    |                |
    |                |
    |                |
    |                |
    |                |
    |                |
    +----------------+
         ||||||||
         ||排线||
         |||||||



 * @param 竖屏 倒立摆放
 *
         ||||||||
         ||排线||
         |||||||
    +----------------+
    |                |
    |                |
    |                |
    |                |
    |                |
    |                |
    |                |
    +----------------+

 */

// LCD重要参数集
typedef struct
{
    uint16_t width; // LCD 宽度
    uint16_t height; // LCD 高度
    uint16_t id; // LCD ID
    uint8_t dir; //横屏还是竖屏控制：0，竖屏；1，横屏。
    uint16_t wramcmd; //开始写gram指令
    uint16_t setxcmd; //设置x坐标指令
    uint16_t setycmd; //设置y坐标指令
} _lcd_dev;

/**
 * @brief LCD的扫描方式定义（扫描方向）
 */
typedef enum {
    L2R_U2D = 0, // 从左到右,从上到下
    L2R_D2U, // 从左到右,从下到上
    R2L_U2D, // 从右到左,从上到下
    R2L_D2U, // 从右到左,从下到上

    U2D_L2R, //从上到下,从左到右
    U2D_R2L, //从上到下,从右到左
    D2U_L2R, //从下到上,从左到右
    D2U_R2L, //从下到上,从右到左
} lcd_scan_type_t;

/**
 * @brief LCD安装的方向定义（设置横屏还是竖屏显示）
 */
typedef enum {
    vertical = 0, // 竖屏
    horizontal, // 横屏
} lcd_display_dir_t;

/**
 * @brief LCD安装的倒置定义（设置屏幕是正着摆放还是倒立摆放的）
 */
typedef enum {
    invert_dis = 0, // 不翻转，屏幕正着摆放
    invert_en, // 翻转，屏幕反着摆放
} lcd_display_invert_t;

/**
 * @brief LCD安装的镜像定义（可用于镜面反射及棱镜的镜像显示，可使画面左右翻转。参照分光棱镜）
 */
typedef enum {
    mirror_dis = 0, // 不镜像，正常显示
    mirror_en, // 镜像，可用于镜面反射及棱镜的镜像显示
} lcd_display_mirror_t;

/**
 * @brief LCD驱动IC型号定义。（需要用户手动配置，不要使用自动识别会出错）
 *        只能保留一个，其余的都注释，做备选
 */
// #define CONFIG_LCD_TYPE_ST7735     1   // LCD驱动固定，型号为ST7735
// #define CONFIG_LCD_TYPE_ST7735S    1   // LCD驱动固定，型号为ST7735S
#define CONFIG_LCD_TYPE_ST7789V 1 // LCD驱动固定，型号为ST7789V，CLK最大频率为80MHz
// #define CONFIG_LCD_TYPE_ILI9341    1   // LCD驱动固定，型号为ILI9341
// #define CONFIG_LCD_TYPE_ILI9488    1   // LCD驱动固定，型号为ILI9488，CLK最大频率为40MHz
// #define CONFIG_LCD_TYPE_ILI9481    1   // LCD驱动固定，型号为ILI9481（配置参数有色偏，且最大CLK只能16MHz。IC已停产，谨慎使用此型号）
// #define CONFIG_LCD_TYPE_HX8357C    1   // LCD驱动固定，型号为HX8357C，CLK最大频率为40MHz（默认配置为RGB565，加快刷屏速度）(HX8357C的驱动完全兼容ST7796S)

/**
 * @brief LCD屏幕像素分辨率定义。（需要用户手动配置）
 *        只能保留一个，其余的都注释，做备选
 */
// #define LCD_PIXEL_SIZE_80_160      1
// #define LCD_PIXEL_SIZE_128_128     1   // 测试 1.44寸 128*128 ST7735方屏，SPI-CLK不能超过30MHz，否则会出现字体花屏，与引线长度有关
// #define LCD_PIXEL_SIZE_135_240     1
// #define LCD_PIXEL_SIZE_240_240     1
#define LCD_PIXEL_SIZE_240_320 1
// #define LCD_PIXEL_SIZE_320_480     1

/**
 * @brief LCD屏幕的安装方向定义（需要用户手动配置）
 *        驱动会根据用户配置自动更改LCD-IC的扫描方向，来适配屏幕的安装方向
 *       （注意：easyIO仅对STxxxx-IC支持横屏适配，且需要用户自行到  simple_gui.c 的 LCD_SetCursor\LCD_SetWindows 修改偏移基地址）
 */
#define LCD_DIR horizontal // vertical、horizontal。竖屏 / 横屏
#define LCD_INVERT invert_en // invert_dis、invert_en。正着放 / 倒立
#define LCD_MIRROR mirror_dis // mirror_dis、mirror_dis。不开启左右镜像显示 / 开启镜像显示（可用于镜面反射及棱镜的镜像显示，可使画面左右翻转。参照分光棱镜）

/**
 * @brief LCD的GPIO定义。（CS尽量使用IOMUX默认的SPI，DC/RST/BLK随意）（CLK、MOSI、MISO在 spi_config.h 中定义）
 */
// SPI引脚。使用默认引脚可以使用IOMUX将GPIO翻转速率提高到80MHz，GPIO矩阵不建议超过26MHz
#define SPI_LCD_PIN_NUM_CS CONFIG_LCD_IONUM_CS // SPI LCD CS引脚
#define SPI_LCD_PIN_NUM_DC CONFIG_LCD_IONUM_DC // D/C线，0-命令，1-数据
#define SPI_LCD_PIN_NUM_RST CONFIG_LCD_IONUM_RST // RST复位引脚。-1为空置不使用。但LCD的RST引脚应上拉接高电平。

// ESP32默认的SPI3引脚。使用默认引脚可以使用IOMUX将GPIO翻转速率提高到80MHz，GPIO矩阵不建议超过26MHz
#define SPI_LCD_PIN_NUM_MISO CONFIG_LCD_IONUM_MISO // SPI默认的MISO引脚
#define SPI_LCD_PIN_NUM_MOSI CONFIG_LCD_IONUM_MOSI // SPI默认的MOSI引脚
#define SPI_LCD_PIN_NUM_CLK CONFIG_LCD_IONUM_CLK // SPI默认的CLK引脚

#define LCD_PIN_NUM_BCKL 5 // 背光PWM引脚。0-关闭，1-打开

#ifdef CONFIG_LCD_HSPI
#define LCD_SPI_SLOT HSPI_HOST // hspi
#elif CONFIG_LCD_VSPI
#define LCD_SPI_SLOT VSPI_HOST // vspi
#endif

//////////////////////////////////////////////////// 下面内容不需要用户二次配置 ////////////////////////////////////////////////////

/* 定义每行的最大像素个数 */
#if LCD_PIXEL_SIZE_80_160
#define LINE_PIXEL_MAX_SIZE 160
#elif LCD_PIXEL_SIZE_128_128
#define LINE_PIXEL_MAX_SIZE 128
#elif LCD_PIXEL_SIZE_135_240
#define LINE_PIXEL_MAX_SIZE 240
#elif LCD_PIXEL_SIZE_240_240
#define LINE_PIXEL_MAX_SIZE 240
#elif LCD_PIXEL_SIZE_240_320
#define LINE_PIXEL_MAX_SIZE 320
#elif LCD_PIXEL_SIZE_320_480
#define LINE_PIXEL_MAX_SIZE 480
#endif

/* 定义每列的最大像素个数 */
#if LCD_PIXEL_SIZE_80_160
#define ROW_PIXEL_MAX_SIZE 80
#elif LCD_PIXEL_SIZE_128_128
#define ROW_PIXEL_MAX_SIZE 128
#elif LCD_PIXEL_SIZE_135_240
#define ROW_PIXEL_MAX_SIZE 135
#elif LCD_PIXEL_SIZE_240_240
#define ROW_PIXEL_MAX_SIZE 240
#elif LCD_PIXEL_SIZE_240_320
#define ROW_PIXEL_MAX_SIZE 240
#elif LCD_PIXEL_SIZE_320_480
#define ROW_PIXEL_MAX_SIZE 320
#endif

#define LCD_DEF_DMA_CHAN SPI_DMA_CH1 // LCD默认使用的SPI-DMA通道
// 为了加快传输速度，每个SPI传输都会发送一堆线。 此定义指定了几行线。 更多意味着更多的内存使用，但设置/完成转帐的开销较小。 确保240可被此整除。
#define PARALLEL_LINES 16
// #define LCD_DMA_MAX_SIZE (PARALLEL_LINES * LINE_PIXEL_MAX_SIZE * 2 + 8) // LCD使用的(PARALLEL_LINES*320*2+8)
#define LCD_DMA_MAX_SIZE (LINE_PIXEL_MAX_SIZE * ROW_PIXEL_MAX_SIZE * sizeof(uint16_t))

// LCD与SPI关联的句柄，通过此来调用SPI总线上的LCD设备
extern spi_device_handle_t LCD_SPI;

/*
    LCD初始化需要的一堆命令/参数值。 它们存储在此结构中。
*/
typedef struct {
    uint8_t cmd;
    uint8_t data[16];
    uint8_t databytes; // No of data in data; bit 7 = delay after set; 0xFF = end of cmds.
} lcd_init_cmd_t;

/*
    下面是关于各型号LCD驱动IC的参数配置。
    每一行分别为 命令、值、值的字节数（0x80代表写入后延时、0xff表示终止传输）
    将数据放入DRAM。 默认情况下，常量数据被放置到DROM中，而DMA无法访问该数据。
*/
#ifdef CONFIG_LCD_TYPE_DEF
DRAM_ATTR static const lcd_init_cmd_t st_init_cmds[] = {
    /* Memory Data Access Control, MX=MV=1, MY=ML=MH=0, RGB=0 */
    { 0x36, { (1 << 5) | (1 << 6) }, 1 },
    /* Interface Pixel Format, 16bits/pixel for RGB/MCU interface */
    { 0x3A, { 0x55 }, 1 },
    /* Porch Setting */
    { 0xB2, { 0x0c, 0x0c, 0x00, 0x33, 0x33 }, 5 },
    /* Gate Control, Vgh=13.65V, Vgl=-10.43V */
    { 0xB7, { 0x45 }, 1 },
    /* VCOM Setting, VCOM=1.175V */
    { 0xBB, { 0x2B }, 1 },
    /* LCM Control, XOR: BGR, MX, MH */
    { 0xC0, { 0x2C }, 1 },
    /* VDV and VRH Command Enable, enable=1 */
    { 0xC2, { 0x01, 0xff }, 2 },
    /* VRH Set, Vap=4.4+... */
    { 0xC3, { 0x11 }, 1 },
    /* VDV Set, VDV=0 */
    { 0xC4, { 0x20 }, 1 },
    /* Frame Rate Control, 60Hz, inversion=0 */
    { 0xC6, { 0x0f }, 1 },
    /* Power Control 1, AVDD=6.8V, AVCL=-4.8V, VDDS=2.3V */
    { 0xD0, { 0xA4, 0xA1 }, 1 },
    /* Positive Voltage Gamma Control */
    { 0xE0, { 0xD0, 0x00, 0x05, 0x0E, 0x15, 0x0D, 0x37, 0x43, 0x47, 0x09, 0x15, 0x12, 0x16, 0x19 }, 14 },
    /* Negative Voltage Gamma Control */
    { 0xE1, { 0xD0, 0x00, 0x05, 0x0D, 0x0C, 0x06, 0x2D, 0x44, 0x40, 0x0E, 0x1C, 0x18, 0x16, 0x19 }, 14 },
    /* Sleep Out */
    { 0x11, { 0 }, 0x80 },
    /* Display On */
    { 0x29, { 0 }, 0x80 },
    { 0, { 0 }, 0xff }
};
#endif

#ifdef CONFIG_LCD_TYPE_ST7735
// st7735。Sitronix 产品列表中，仅有 st7735S、st7735SV，而卖家的介绍一般都不带后缀，使用时小心并加以区分
DRAM_ATTR static const lcd_init_cmd_t st_7735_init_cmds[] = {
    /* Normal mode */
    { 0xB1, { 0x01, 0x2C, 0x2D }, 3 },
    /* Idle mode */
    { 0xB2, { 0x01, 0x2C, 0x2D }, 3 },
    /* Partial mode */
    { 0xB3, { 0x01, 0x2C, 0x2D, 0x01, 0x2C, 0x2D }, 6 },
    /* Column inversion */
    { 0xB4, { 0x07 }, 1 },
    /* AVDD GVDD */
    { 0xC0, { 0xA2, 0x02, 0x84 }, 3 },
    /* VGH VGL */
    { 0xC1, { 0xC5 }, 1 },
    /* Normal Mode */
    { 0xC2, { 0x0A, 0x00 }, 2 },
    /* Idle */
    { 0xC3, { 0x8A, 0x2A }, 2 },
    /* Partial+Full */
    { 0xC4, { 0x8A, 0xEE }, 2 },
    /* VCOM */
    { 0xC5, { 0x0E }, 1 },
    /* Positive Voltage Gamma Control */
    { 0xE0, { 0x0F, 0x1A, 0x0F, 0x18, 0x2F, 0x28, 0x20, 0x22, 0x1F, 0x1B, 0x23, 0x37, 0x00, 0x07, 0x02, 0x10 }, 16 },
    /* Negative Voltage Gamma Control */
    { 0xE1, { 0x0F, 0x1B, 0x0F, 0x17, 0x33, 0x2C, 0x29, 0x2E, 0x30, 0x30, 0x39, 0x3F, 0x00, 0x07, 0x03, 0x10 }, 16 },
    /* ?? */
    //{0xFC, {0x80}, 1},
    /* ?? */
    //{0x3A, {0x05}, 1},
    /* ?? */
    { 0x36, { 0xC8 }, 1 },
    /* Enable test command */
    { 0xF0, { 0x01 }, 1 },
    /* Disable ram power save mode */
    { 0xF6, { 0x00 }, 1 },
    /* 65k mode */
    { 0x3A, { 0x05 }, 1 },
    /* Display Inversion On。st7735S不需要反转显示 */
    //{0x21, {0}, 0x80},
    /* Sleep Out */
    { 0x11, { 0 }, 0x80 },
    /* Display On */
    { 0x29, { 0 }, 0x80 },
    { 0x2A, { 0x00, 0x00, 0X00, 0X7F }, 4 },
    { 0x2B, { 0x00, 0x00, 0X00, 0X9F }, 4 },
    { 0, { 0 }, 0xff }
};
#endif

#ifdef CONFIG_LCD_TYPE_ST7735S
// st7735S。Sitronix 产品列表中，仅有 st7735S、st7735SV，而卖家的介绍一般都不带后缀，使用时小心并加以区分
DRAM_ATTR static const lcd_init_cmd_t st_7735S_init_cmds[] = {
    /* Normal mode */
    { 0xB1, { 0x05, 0x3C, 0x3C }, 3 },
    /* Idle mode */
    { 0xB2, { 0x05, 0x3C, 0x3C }, 3 },
    /* Partial mode */
    { 0xB3, { 0x05, 0x3C, 0x3C, 0x05, 0x3C, 0x3C }, 6 },
    /* Dot inversion */
    { 0xB4, { 0x03 }, 1 },
    /* AVDD GVDD */
    { 0xC0, { 0xAB, 0x0B, 0x04 }, 3 },
    /* VGH VGL */
    { 0xC1, { 0x05 }, 1 },
    /* Normal Mode */
    { 0xC2, { 0x0D, 0x00 }, 2 },
    /* Idle */
    { 0xC3, { 0x8D, 0x6A }, 2 },
    /* Partial+Full */
    { 0xC4, { 0x8D, 0xEE }, 2 },
    /* VCOM */
    { 0xC5, { 0x0F }, 1 },
    /* Positive Voltage Gamma Control */
    { 0xE0, { 0x07, 0x0E, 0x08, 0x07, 0x10, 0x07, 0x02, 0x07, 0x09, 0x0F, 0x25, 0x36, 0x00, 0x08, 0x04, 0x10 }, 16 },
    /* Negative Voltage Gamma Control */
    { 0xE1, { 0x0A, 0x0D, 0x08, 0x07, 0x0F, 0x07, 0x02, 0x07, 0x09, 0x0F, 0x25, 0x35, 0x00, 0x09, 0x04, 0x10 }, 16 },
    /* ?? */
    { 0xFC, { 0x80 }, 1 },
    /* ?? */
    { 0x3A, { 0x05 }, 1 },
    /* ?? */
    { 0x36, { 0x78 }, 1 },
    /* Display Inversion On。st7735S不需要反转显示 */
    //{0x21, {0}, 0x80},
    /* Sleep Out */
    { 0x11, { 0 }, 0x80 },
    /* Display On */
    { 0x29, { 0 }, 0x80 },
    { 0x2A, { 0x00, 0x1A, 0X00, 0X69 }, 4 },
    { 0x2B, { 0x00, 0x01, 0X00, 0XA0 }, 4 },
    { 0, { 0 }, 0xff }
};
#endif

#ifdef CONFIG_LCD_TYPE_ST7789V
// st7789V
DRAM_ATTR static const lcd_init_cmd_t st_7789V_init_cmds[] = {
    /* Memory Data Access Control, MX=MV=1, MY=ML=MH=0, RGB=0 */
    // { 0x36, { (1 << 4) | (1 << 5) | (1 << 6) }, 1 },
    // /* Interface Pixel Format, 16bits/pixel for RGB/MCU interface */
    // { 0x3A, { 0x05 }, 1 },
    // /* Porch Setting */
    // { 0xB2, { 0x0c, 0x0c, 0x00, 0x33, 0x33 }, 5 },
    // /* Gate Control, Vgh=13.65V, Vgl=-10.43V */
    // { 0xB7, { 0x35 }, 1 },
    // /* VCOM Setting, VCOM=1.175V */
    // { 0xBB, { 0x19 }, 1 },
    // /* LCM Control, XOR: BGR, MX, MH */
    // { 0xC0, { 0x2C }, 1 },
    // /* VDV and VRH Command Enable, enable=1 */
    // //{0xC2, {0x01, 0xff}, 2},
    // { 0xC2, { 0x01 }, 1 },
    // /* VRH Set, Vap=4.4+... */
    // { 0xC3, { 0x12 }, 1 },
    // /* VDV Set, VDV=0 */
    // { 0xC4, { 0x20 }, 1 },
    // /* Frame Rate Control, 60Hz, inversion=0 */
    // { 0xC6, { 0x0f }, 1 },
    // /* Power Control 1, AVDD=6.8V, AVCL=-4.8V, VDDS=2.3V */
    // { 0xD0, { 0xA4, 0xA1 }, 1 },
    // /* Positive Voltage Gamma Control */
    // { 0xE0, { 0xD0, 0x04, 0x0D, 0x11, 0x13, 0x2B, 0x3F, 0x54, 0x4C, 0x18, 0x0D, 0x0B, 0x1F, 0x23 }, 14 },
    // /* Negative Voltage Gamma Control */
    // { 0xE1, { 0xD0, 0x04, 0x0C, 0x11, 0x13, 0x2C, 0x3F, 0x44, 0x51, 0x2F, 0x1F, 0x1F, 0x20, 0x23 }, 14 },
    // /* Display Inversion On */
    // { 0x21, { 0 }, 0x80 },
    // /* Sleep Out */
    // { 0x11, { 0 }, 0x80 },
    // /* Display On */
    // { 0x29, { 0 }, 0x80 },

    { 0x11, { 0 }, 0x80 },

    { 0x36, { 0x00 }, 1 },
    { 0x3a, { 0x05 }, 1 },

    { 0xb2, { 0x0c, 0x0c, 0x00, 0x33, 0x33 }, 5 },
    { 0xb7, { 0x35 }, 1 },

    { 0xbb, { 0x20 }, 1 },
    { 0xc0, { 0x2c }, 1 },
    { 0xc2, { 0x01 }, 1 },
    { 0xc3, { 0x0b }, 1 },
    { 0xc4, { 0x20 }, 1 },
    { 0xc6, { 0x0f }, 1 },
    { 0xd0, { 0xa4, 0xa1 }, 2 },

    { 0xe0, { 0xd0, 0x03, 0x09, 0x0e, 0x11, 0x3d, 0x47, 0x55, 0x53, 0x1a, 0x16, 0x14, 0x1f, 0x22 }, 14 },
    { 0xe1, { 0xd0, 0x02, 0x08, 0x0d, 0x12, 0x2c, 0x43, 0x55, 0x53, 0x1e, 0x1b, 0x19, 0x20, 0x22 }, 14 },

    { 0x29, { 0 }, 0x80 },
    { 0, { 0 }, 0xff },
};
#endif

#ifdef CONFIG_LCD_TYPE_ILI9341
// ili9341
DRAM_ATTR static const lcd_init_cmd_t ili_9341_init_cmds[] = {
    /* Power contorl B, power control = 0, DC_ENA = 1 */
    { 0xCF, { 0x00, 0x83, 0X30 }, 3 },
    /* Power on sequence control,
     * cp1 keeps 1 frame, 1st frame enable
     * vcl = 0, ddvdh=3, vgh=1, vgl=2
     * DDVDH_ENH=1
     */
    { 0xED, { 0x64, 0x03, 0X12, 0X81 }, 4 },
    /* Driver timing control A,
     * non-overlap=default +1
     * EQ=default - 1, CR=default
     * pre-charge=default - 1
     */
    { 0xE8, { 0x85, 0x01, 0x79 }, 3 },
    /* Power control A, Vcore=1.6V, DDVDH=5.6V */
    { 0xCB, { 0x39, 0x2C, 0x00, 0x34, 0x02 }, 5 },
    /* Pump ratio control, DDVDH=2xVCl */
    { 0xF7, { 0x20 }, 1 },
    /* Driver timing control, all=0 unit */
    { 0xEA, { 0x00, 0x00 }, 2 },
    /* Power control 1, GVDD=4.75V */
    { 0xC0, { 0x26 }, 1 },
    /* Power control 2, DDVDH=VCl*2, VGH=VCl*7, VGL=-VCl*3 */
    { 0xC1, { 0x11 }, 1 },
    /* VCOM control 1, VCOMH=4.025V, VCOML=-0.950V */
    { 0xC5, { 0x35, 0x3E }, 2 },
    /* VCOM control 2, VCOMH=VMH-2, VCOML=VML-2 */
    { 0xC7, { 0xBE }, 1 },
    /* Memory access contorl, MX=MY=0, MV=1, ML=0, BGR=1, MH=0 */
    { 0x36, { 0x28 }, 1 },
    /* Pixel format, 16bits/pixel for RGB/MCU interface */
    { 0x3A, { 0x55 }, 1 },
    /* Frame rate control, f=fosc, 70Hz fps */
    { 0xB1, { 0x00, 0x1B }, 2 },
    /* Enable 3G, disabled */
    { 0xF2, { 0x08 }, 1 },
    /* Gamma set, curve 1 */
    { 0x26, { 0x01 }, 1 },
    /* Positive gamma correction */
    { 0xE0, { 0x1F, 0x1A, 0x18, 0x0A, 0x0F, 0x06, 0x45, 0X87, 0x32, 0x0A, 0x07, 0x02, 0x07, 0x05, 0x00 }, 15 },
    /* Negative gamma correction */
    { 0XE1, { 0x00, 0x25, 0x27, 0x05, 0x10, 0x09, 0x3A, 0x78, 0x4D, 0x05, 0x18, 0x0D, 0x38, 0x3A, 0x1F }, 15 },
    /* Column address set, SC=0, EC=0xEF */
    { 0x2A, { 0x00, 0x00, 0x00, 0xEF }, 4 },
    /* Page address set, SP=0, EP=0x013F */
    { 0x2B, { 0x00, 0x00, 0x01, 0x3f }, 4 },
    /* Memory write */
    { 0x2C, { 0 }, 0 },
    /* Entry mode set, Low vol detect disabled, normal display */
    { 0xB7, { 0x07 }, 1 },
    /* Display function control */
    { 0xB6, { 0x0A, 0x82, 0x27, 0x00 }, 4 },
    /* Sleep out */
    { 0x11, { 0 }, 0x80 },
    /* Display on */
    { 0x29, { 0 }, 0x80 },
    { 0, { 0 }, 0xff },
};
#endif

#ifdef CONFIG_LCD_TYPE_ILI9488
// ili9488
DRAM_ATTR static const lcd_init_cmd_t ili_9488_init_cmds[] = {
    /* Positive gamma correction */
    { 0xE0, { 0x00, 0x03, 0x09, 0x08, 0x16, 0x0A, 0x3F, 0x78, 0x4C, 0x09, 0x0A, 0x08, 0x16, 0x1A, 0x0F }, 15 },
    /* Negative gamma correction */
    { 0XE1, { 0x00, 0x16, 0x19, 0x03, 0x0F, 0x05, 0x32, 0x45, 0x46, 0x04, 0x0E, 0x0D, 0x35, 0x37, 0x0F }, 15 },
    /* Power control 1, GVDD=4.75V */
    { 0xC0, { 0x17, 0x15 }, 2 },
    /* Power control 2, DDVDH=VCl*2, VGH=VCl*7, VGL=-VCl*3 */
    { 0xC1, { 0x41 }, 1 },
    /* VCOM control 1, VCOMH=4.025V, VCOML=-0.950V */
    { 0xC5, { 0x00, 0x12, 0x80 }, 3 },
    /* Memory access contorl, MX=MY=0, MV=1, ML=0, BGR=1, MH=0 */
    { 0x36, { 0x48 }, 1 },
    /* Pixel format, 18bits/pixel for RGB/MCU interface */
    { 0x3A, { 0x66 }, 1 },
    /* Interface Mode Control */
    { 0xB0, { 0x00 }, 1 },
    /* Frame rate control, f=fosc, 60Hz fps */
    { 0xB1, { 0xA0 }, 1 },
    /* Display Inversion Control,2-dot */
    { 0xB4, { 0x02 }, 1 },
    /* Display function control,RGB/MCU Interface Control */
    { 0xB6, { 0x02, 0x02 }, 2 },
    /* Set Image Functio,Disable 24 bit data */
    { 0xE9, { 0x00 }, 1 },
    /* Adjust Control,D7 stream, loose */
    { 0xF7, { 0xA9, 0x51, 0x2C, 0x82 }, 4 },
    /* Sleep out */
    { 0x11, { 0 }, 0x80 },
    /* Display on */
    { 0x29, { 0 }, 0x80 },
    { 0, { 0 }, 0xff },
};
#endif

#ifdef CONFIG_LCD_TYPE_ILI9481
// 换了一家屏幕，无频闪、波纹，但最大CLK只能16MHz，且有比较严重色偏
// ili9481
DRAM_ATTR static const lcd_init_cmd_t ili_9481_init_cmds[] = {
    /* Soft reset */
    //{0x01, {0}, 0x80},
    /* Power_Setting,
     * 07  VC[2:0] Sets the ratio factor of Vci to generate the reference voltages Vci1
     * 41  BT[2:0] Sets the Step up factor and output voltage level from the reference voltages Vci1
     * 1f  17   1C  VRH[3:0]: Sets the factor to generate VREG1OUT from VCILVL
     */
    { 0xD0, { 0x07, 0x42, 0x1B }, 3 },
    /* VCOM Control,
     * 00
     * 1A   VCM [6:0] is used to set factor to generate VCOMH voltage from the reference voltage VREG1OUT  15    09
     * 1F   VDV[4:0] is used to set the VCOM alternating amplitude in the range of VREG1OUT x 0.70 to VREG1OUT   1F   18
     */
    { 0xD1, { 0x00, 0x05, 0x0C }, 3 },
    /* Power_Setting for Normal Mode */
    { 0xD2, { 0x01, 0x11 }, 2 },
    /* Panel Driving Setting,
     * 1//00  REV  SM  GS
     * 2//NL[5:0]: Sets the number of lines to drive the LCD at an interval of 8 lines.
     * 3//SCN[6:0]
     * 4//PTV: Sets the Vcom output in non-display area drive period
     * 5//NDL: Sets the source output level in non-display area.  PTG: Sets the scan mode in non-display area.
     */
    { 0xC0, { 0x00, 0x3B, 0x00, 0x02, 0x11 }, 5 },
    /* Frame Rate  03   02 */
    { 0xC5, { 0x02 }, 1 },
    /* GAMMA SETTING */
    { 0xC8, { 0x00, 0x22, 0x37, 0x20, 0x04, 0x00, 0x04, 0x55, 0x77, 0x02, 0x04, 0x00 }, 12 },
    /* 倒装设置   NG  8C:出来两行   CA：出来一个点 */
    //{0x36, {0x8C}, 1},
    { 0x36, { 0x0A }, 1 },
    /* Pixel format, 18bits/pixel for RGB/MCU interface */
    { 0x3A, { 0x66 }, 1 },
    /* ?? */
    //{0x2A, {0x00, 0x00, 0x01, 0x3F}, 4},
    /* ?? */
    //{0x2B, {0x00, 0x00, 0x01, 0xDF}, 4},
    /* Sleep out */
    { 0x11, { 0 }, 0x80 },
    /* Display on */
    { 0x29, { 0 }, 0x80 },
    { 0, { 0 }, 0xff },
};
#endif

#ifdef CONFIG_LCD_TYPE_HX8357C
// （320x480，16.7M）支持SPI-RGB565（2字节）、SPI-RGB666（3字节）。（RGB666方式可直接使用ili9488的配置项。这里配置为RGB565，加快刷屏速度）
// HX8357C
DRAM_ATTR static const lcd_init_cmd_t hx_8357c_init_cmds[] = {
    /* enable extention command */
    { 0xB9, { 0xFF, 0x83, 0x57 }, 3 },
    // 如初始化不正常，请在这里插入延时20ms
    /* set VCOM voltage related register */
    { 0xB6, { 0x2C }, 1 },
    // 如初始化不正常，请在这里插入退出休眠，并延时20ms
    /* TE ON */
    { 0x35, { 0 }, 0x80 },
    /* Interface pixel format: 262K, 16bits/pixel */
    { 0x3A, { 0x55 }, 1 },
    /* set internal oscillator: 70Hz */
    { 0xB0, { 0x68 }, 1 },
    ///* set panel characteristic: normally white panel; BGR; G1->G480; S960->S1 */
    //{0xCC, {0x09}, 1},
    /* set panel characteristic: normally white panel; RGB; G1->G480; S1->S960 */
    { 0xCC, { 0x00 }, 1 },
    /* set power control */
    { 0xB1, { 0x00, 0x15, 0x1C, 0x1C, 0x83, 0x48 }, 6 },
    /* xxx */
    { 0xC0, { 0x50, 0x50, 0x01, 0x3C, 0x1E, 0x08 }, 6 },
    /* set display cycle register */ // 下面这两处分开的，向0x00写数据，应该是模组厂家的源码有误，已经更正
    //{0xB4, {0x02, 0x40}, 2},
    { 0xB4, { 0x02, 0x40, 0x00, 0x2A, 0x2A, 0x0D, 0x78 }, 7 },
    /* NOP */
    //{0x00, {0x2A, 0x2A, 0x0D, 0x78}, 4},
    /* SETGamma: set gamma curve */
    /*{0xE0, {0x02, 0x08, 0x11, 0x23, 0x2C, 0x40, 0x4A, 0x52, \
            0x48, 0x41, 0x3C, 0x33, 0x2E, 0x28, 0x27, 0x1B, \
            0x02, 0x08, 0x11, 0x23, 0x2C, 0x40, 0x4A, 0x52, \
            0x48, 0x41, 0x3C, 0x33, 0x2E, 0x28 ,0x27, 0x1B}, 32},*/
    { 0xE0, { 0x02, 0x08, 0x11, 0x23, 0x2C, 0x40, 0x4A, 0x52, 0x48, 0x41, 0x3C, 0x33, 0x2E, 0x28, 0x27, 0x1B, 0x02, 0x08, 0x11, 0x23, 0x2C, 0x40, 0x4A, 0x52, 0x48, 0x41, 0x3C, 0x33, 0x2E, 0x28, 0x27, 0x1B, 0x00, 0x01 }, 34 },
    /* NOP */
    //{0x00, {0x01}, 1},
    /* Sleep out */
    { 0x11, { 0 }, 0x80 },
    /* Display on */
    { 0x29, { 0 }, 0x80 },
    { 0, { 0 }, 0xff },
};
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 显示命令
#ifdef CONFIG_LCD_TYPE_ST7789V
#define MADCTL_MY 0x80 ///< Bottom to top 1 << 7
#define MADCTL_MX 0x40 ///< Right to left 1 << 6
#define MADCTL_MV 0x20 ///< Reverse Mode 1 << 5
#define MADCTL_ML 0x10 ///< LCD refresh Bottom to top 1 << 4
#define MADCTL_RGB 0x00 ///< Red-Green-Blue pixel order
#define MADCTL_BGR 0x08 ///< Blue-Green-Red pixel order 1 << 3
#define MADCTL_MH 0x04 ///< LCD refresh right to left 1 << 2

#define ST7789_NOP 0x00
#define ST7789_SWRESET 0x01
#define ST7789_RDDID 0x04
#define ST7789_RDDST 0x09

#define ST7789_RDDPM 0x0A // Read display power mode
#define ST7789_RDD_MADCTL 0x0B // Read display MADCTL
#define ST7789_RDD_COLMOD 0x0C // Read display pixel format
#define ST7789_RDDIM 0x0D // Read display image mode
#define ST7789_RDDSM 0x0E // Read display signal mode
#define ST7789_RDDSR 0x0F // Read display self-diagnostic result (ST7789V)

#define ST7789_SLPIN 0x10 // 进入休眠模式
#define ST7789_SLPOUT 0x11 // 退出休眠模式
#define ST7789_PTLON 0x12
#define ST7789_NORON 0x13

#define ST7789_INVOFF 0x20
#define ST7789_INVON 0x21
#define ST7789_GAMSET 0x26 // Gamma set
#define ST7789_DISPOFF 0x28
#define ST7789_DISPON 0x29
#define ST7789_CASET 0x2A
#define ST7789_RASET 0x2B
#define ST7789_RAMWR 0x2C
#define ST7789_RGBSET 0x2D // Color setting for 4096, 64K and 262K colors
#define ST7789_RAMRD 0x2E

#define ST7789_PTLAR 0x30
#define ST7789_VSCRDEF 0x33 // Vertical scrolling definition (ST7789V)
#define ST7789_TEOFF 0x34 // Tearing effect line off
#define ST7789_TEON 0x35 // Tearing effect line on
#define ST7789_MADCTL 0x36 // Memory data access control
#define ST7789_IDMOFF 0x38 // Idle mode off
#define ST7789_IDMON 0x39 // Idle mode on
#define ST7789_RAMWRC 0x3C // Memory write continue (ST7789V)
#define ST7789_RAMRDC 0x3E // Memory read continue (ST7789V)
#define ST7789_COLMOD 0x3A

#define ST7789_RAMCTRL 0xB0 // RAM control
#define ST7789_RGBCTRL 0xB1 // RGB control
#define ST7789_PORCTRL 0xB2 // Porch control
#define ST7789_FRCTRL1 0xB3 // Frame rate control
#define ST7789_PARCTRL 0xB5 // Partial mode control
#define ST7789_GCTRL 0xB7 // Gate control
#define ST7789_GTADJ 0xB8 // Gate on timing adjustment
#define ST7789_DGMEN 0xBA // Digital gamma enable
#define ST7789_VCOMS 0xBB // VCOMS setting
#define ST7789_LCMCTRL 0xC0 // LCM control
#define ST7789_IDSET 0xC1 // ID setting
#define ST7789_VDVVRHEN 0xC2 // VDV and VRH command enable
#define ST7789_VRHS 0xC3 // VRH set
#define ST7789_VDVSET 0xC4 // VDV setting
#define ST7789_VCMOFSET 0xC5 // VCOMS offset set
#define ST7789_FRCTR2 0xC6 // FR Control 2
#define ST7789_CABCCTRL 0xC7 // CABC control
#define ST7789_REGSEL1 0xC8 // Register value section 1
#define ST7789_REGSEL2 0xCA // Register value section 2
#define ST7789_PWMFRSEL 0xCC // PWM frequency selection
#define ST7789_PWCTRL1 0xD0 // Power control 1
#define ST7789_VAPVANEN 0xD2 // Enable VAP/VAN signal output
#define ST7789_CMD2EN 0xDF // Command 2 enable
#define ST7789_PVGAMCTRL 0xE0 // Positive voltage gamma control
#define ST7789_NVGAMCTRL 0xE1 // Negative voltage gamma control
#define ST7789_DGMLUTR 0xE2 // Digital gamma look-up table for red
#define ST7789_DGMLUTB 0xE3 // Digital gamma look-up table for blue
#define ST7789_GATECTRL 0xE4 // Gate control
#define ST7789_SPI2EN 0xE7 // SPI2 enable
#define ST7789_PWCTRL2 0xE8 // Power control 2
#define ST7789_EQCTRL 0xE9 // Equalize time control
#define ST7789_PROMCTRL 0xEC // Program control
#define ST7789_PROMEN 0xFA // Program mode enable
#define ST7789_NVMSET 0xFC // NVM setting
#define ST7789_PROMACT 0xFE // Program action
#endif

/**
 * @brief  向LCD发送1个字节的命令（D/C线电平为0）
 *       - 使用spi_device_polling_transmit，它等待直到传输完成。
 *       - 发送时同时设置D/C线为0，传输命令
 *       - 例：lcd_cmd(LCD_SPI, 0x04);
 *
 * @param  spi LCD与SPI关联的句柄，通过此来调用SPI总线上的LCD设备
 * @param  cmd 向LCD发送的1个字节命令内容
 *
 * @return
 *     - none
 */
void lcd_cmd(spi_device_handle_t spi, const uint8_t cmd);

/**
 * @brief  向LCD发送长度为len个字节的数据（D/C线电平为1）
 *       - 发送时同时设置D/C线为1，传输数据
 *       - 例：lcd_data(LCD_SPI, dataBuf, 10);
 *
 * @param  spi LCD与SPI关联的句柄，通过此来调用SPI总线上的LCD设备
 * @param  data 要发送数据的指针
 * @param  len 发送的字节数
 *
 * @return
 *     - none
 */
void lcd_data(spi_device_handle_t spi, const uint8_t* data, int len);

/**
 * @brief  向LCD发送单点16Bit的像素数据，（根据驱动IC的不同，可能为2或3个字节，需要转换RGB565、RGB666）
 *       - ili9488\ili9481 这类IC，SPI总线仅能使用RGB666-18Bit/像素，分3字节传输。而不能使用16Bit/像素，分2字节传输。（0x3A寄存器）
 *       - 例：lcd_data16(LCD_SPI, RED);
 *
 * @param  spi LCD与SPI关联的句柄，通过此来调用SPI总线上的LCD设备
 * @param  data 要发送的单点像素数据，uint16_t的RGB565
 *
 * @return
 *     - none
 */
void lcd_data16(spi_device_handle_t spi, uint16_t data);

/**
 * @brief  以SPI方式驱动LCD初始化函数
 *       - 过程包括：关联 SPI总线及LCD设备、驱动IC的参数配置、点亮背光、设置LCD的安装方向、设置屏幕分辨率、扫描方向、初始化显示区域的大小
 *       - （注意：普通GPIO最大只能30MHz，而IOMUX默认的SPI，CLK最大可以设置到80MHz）
 *       - 例：spi_lcd_init(SPI2_HOST, 60*1000*1000, SPI_LCD_PIN_NUM_CS);
 *
 * @param  host_id SPI端口号。SPI1_HOST / SPI2_HOST / SPI3_HOST
 * @param  clk_speed LCD设备的SPI速度（注意：普通GPIO最大只能30MHz，而IOMUX默认的SPI，CLK最大可以设置到80MHz）
 * @param  cs_io_num CS端口号，尽量使用IOMUX默认的IO
 *
 * @return
 *     - none
 */
void spi_lcd_init(spi_host_device_t host_id, uint32_t clk_speed, gpio_num_t cs_io_num);

/**
 * @brief  配置SPIx主机模式，配置DMA通道、DMA字节大小，及 MISO、MOSI、CLK的引脚。
 *       - （注意：普通GPIO最大只能30MHz，而IOMUX默认的SPI-IO，CLK最大可以设置到80MHz）
 *       - 例：spi_master_init(SPI2_HOST, LCD_DEF_DMA_CHAN, LCD_DMA_MAX_SIZE, SPI_LCD_PIN_NUM_MISO, SPI_LCD_PIN_NUM_MOSI, SPI_LCD_PIN_NUM_CLK);
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
void spi_master_init(spi_host_device_t host_id, int dma_chan, uint32_t max_tran_size, gpio_num_t miso_io_num, gpio_num_t mosi_io_num, gpio_num_t clk_io_num);

/**
 * @brief 初始化背光PWM GPIO
 *
 */
void lcd_backlight_init(void);

/**
 * @brief 设置背光
 *
 * @param value
 */
void lcd_backlight_set(uint16_t value);

void LCD_WR_REG(uint8_t reg_addr);
void LCD_WR_DATA(const uint8_t* data, int len);
void LCD_WR_DATA16(uint16_t data);
void LCD_WriteReg(uint8_t LCD_Reg, uint8_t LCD_RegValue);
void LCD_WriteRAM_Prepare(void);

#endif
