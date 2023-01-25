#ifndef MAIN_ST7789_H_
#define MAIN_ST7789_H_

#include "esp_system.h"
#include "spi_lcd.h"

// 模式选择
#define ST7789_DIRECT_MODE 0 // 直接显示访问模式（无帧缓冲区）
#define ST7789_BUFFER_MODE 1 // 更改帧缓冲区以便稍后加载到显示器的模式
#define ST7789_MODE ST7789_BUFFER_MODE // 使用显存模式 占用内存 刷新快
// #define ST7789_MODE ST7789_DIRECT_MODE // 直接显示模式 慢 

// LCD 旋转角度
#define DIRECTION0		0
#define DIRECTION90		1
#define DIRECTION180	2
#define DIRECTION270	3

extern _lcd_dev lcddev;


// 显示初始化程序
void st7789_init();

// 
void st7789_DisplayOff(void);

// 该程序为显示器的 1 个像素着色
void st7789_DrawPixel(int16_t x, int16_t y, uint16_t color);

// 用颜色填充矩形的过程
void st7789_FillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);

#if (ST7789_MODE == ST7789_BUFFER_MODE)
// 该过程从帧缓冲区更新显示
void st7789_update(void);

// 复制显存数据到指定内存
void st7789_getScreenData(uint16_t *pBuff);

// 该过程返回一个像素的颜色
uint16_t st7789_GetPixel(int16_t x, int16_t y);
#endif


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
void LCD_SetCursor(uint16_t Xpos, uint16_t Ypos);

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
void LCD_SetWindows(uint16_t xStar, uint16_t yStar, uint16_t xEnd, uint16_t yEnd);


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
void LCD_Scan_Dir(uint8_t dir);

#endif /* MAIN_ST7789_H_ */
