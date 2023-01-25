//------------------------------------------------------------------------------
// This is Open source software. You can place this code on your site, but don't
// forget a link to my YouTube-channel: https://www.youtube.com/channel/UChButpZaL5kUUl_zTyIDFkQ
// Это программное обеспечение распространяется свободно. Вы можете размещать
// его на вашем сайте, но не забудьте указать ссылку на мой YouTube-канал
// "Электроника в объектике" https://www.youtube.com/channel/UChButpZaL5kUUl_zTyIDFkQ
// Автор: Надыршин Руслан / Nadyrshin Ruslan
//------------------------------------------------------------------------------
#ifndef _DISPCOLOR_H
#define _DISPCOLOR_H

#include "font.h"


#define RGB565(r, g, b)         (((r & 0xF8) << 8) | ((g & 0xFC) << 3) | ((b & 0xF8) >> 3))

typedef union _uRGB565
{
	uint16_t value;
	struct
	{
		uint16_t b	:5;
		uint16_t g	:6;
		uint16_t r	:5;
	} rgb_color;
} uRGB565;


//画笔颜色
#define WHITE 0xFFFF
#define BLACK 0x0000
#define BLUE 0x001F
#define BRED 0xF81F
#define GBLUE 0x07FF
#define RED 0xF800
#define MAGENTA 0xF81F
#define GREEN 0x07E0
#define CYAN 0x7FFF
#define YELLOW 0xFFE0
#define BROWN 0xBC40 //棕色
#define BRRED 0xFC07 //棕红色
#define GRAY 0x8430 //灰色
// GUI颜色

#define DARKBLUE 0x01CF //深蓝色
#define LIGHTBLUE 0x7D7C //浅蓝色
#define GRAYBLUE 0x5458 //灰蓝色
//以上三色为PANEL的颜色

#define LIGHTGREEN 0x841F //浅绿色
#define LIGHTGRAY 0xEF5B //浅灰色(PANNEL)
#define LGRAY 0xC618 //浅灰色(PANNEL),窗体背景色

#define LGRAYBLUE 0xA651 //浅灰蓝色(中间层颜色)
#define LBBLUE 0x2B12 //浅棕蓝色(选择条目的反色)


//程序初始化彩色显示
void dispcolor_Init();
//程序初关闭显示
void dispcolor_DisplayOff(void);
//以像素为单位返回显示大小的函数
uint16_t dispcolor_getWidth();
uint16_t dispcolor_getHeight();
//该过程清除屏幕（将其涂黑）
void dispcolor_ClearScreen(void);
//该程序设置显示器的亮度
void dispcolor_SetBrightness(uint16_t Value);
//用颜色填充矩形的过程
void dispcolor_FillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
//程序颜色显示的 1 个像素
void dispcolor_DrawPixel(int16_t X, int16_t Y, uint16_t color);
//该过程返回像素的颜色
uint16_t dispcolor_GetPixel(int16_t x, int16_t y);
//该过程用颜色绘制屏幕
void dispcolor_FillScreen(uint16_t color);
//该过程从帧缓冲区更新显示
void dispcolor_Update(void);
//例程在显示器上画一条直线
void dispcolor_DrawLine(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color);
//例程在显示器上绘制一个矩形
void dispcolor_DrawRectangle(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color);
//例程在显示器上绘制一个实心矩形
void dispcolor_DrawRectangleFilled(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t fillcolor);
//例程在显示器上绘制一个圆圈。 x0 和 y0 -圆心坐标
void dispcolor_DrawCircle(int16_t x0, int16_t y0, int16_t radius, uint16_t color);
// 该例程在显示器上绘制一个实心圆圈。 x0 和 y0 -圆心坐标
void dispcolor_DrawCircleFilled(int16_t x0, int16_t y0, int16_t radius, uint16_t fillcolor);
//用于将字符串 Str 中的文本输出到显示器的函数。返回线的宽度（以像素为单位）
int16_t dispcolor_DrawString(int16_t X, int16_t Y, uint8_t FontID, uint8_t *Str, uint16_t TextColor);
//用于将字符串 Str 中的文本输出到显示器的函数。返回线的宽度（以像素为单位）
int16_t dispcolor_DrawString_Bg(int16_t X, int16_t Y, uint8_t FontID, uint8_t* Str, uint16_t TextColor, uint16_t BgColor);
//该过程显示一个字符串
int16_t dispcolor_printf(int16_t X, int16_t Y, uint8_t FontID, uint16_t TextColor, const char *args, ...);
//该过程显示一个字符串
int16_t dispcolor_printf_Bg(int16_t X, int16_t Y, uint8_t FontID, uint16_t TextColor, uint16_t BgColor, const char *args, ...);
//该函数以像素为单位返回线的宽度
int16_t dispcolor_getStrWidth(uint8_t FontID, char *Str);
//该函数以像素为单位返回格式化字符串的宽度
int16_t dispcolor_getFormatStrWidth(uint8_t FontID, const char *args, ...);
//该过程将屏幕上图像的亮度降低 ~ 2 倍
void dispcolor_screenDark(void);
// 复制显存数据到指定内存
void dispcolor_getScreenData(uint16_t *pBuff);


#endif
