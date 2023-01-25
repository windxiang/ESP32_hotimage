#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "spi_lcd.h"
#include "thermalimaging.h"
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief 交换 2 个 int16_t 值的过程
 *
 * @param pValue1
 * @param pValue2
 */
static void SwapInt16Values(int16_t* pValue1, int16_t* pValue2)
{
    int16_t TempValue = *pValue1;
    *pValue1 = *pValue2;
    *pValue2 = TempValue;
}

/**
 * @brief 程序初始化彩色显示
 *
 */
void dispcolor_Init()
{
    // 初始化显示
    st7789_init();
}

/**
 * @brief 关闭LCD
 *
 */
void dispcolor_DisplayOff(void)
{
    dispcolor_SetBrightness(0);
    dispcolor_ClearScreen();
    st7789_DisplayOff();
}

/**
 * @brief 得到屏幕分辨率
 *
 * @return uint16_t
 */
uint16_t dispcolor_getWidth()
{
    return lcddev.width;
}

uint16_t dispcolor_getHeight()
{
    return lcddev.height;
}

/**
 * @brief 设置显示屏的亮度
 *
 * @param Value
 */
void dispcolor_SetBrightness(uint16_t Value)
{
    lcd_backlight_set(Value);
}

/**
 * @brief 绘制一个点
 *
 * @param x 起始横坐标
 * @param y 起始纵坐标
 * @param color 颜色
 */
void dispcolor_DrawPixel(int16_t x, int16_t y, uint16_t color)
{
    st7789_DrawPixel(x, y, color);
}

/**
 * @brief 返回一个像素的颜色
 *
 * @param x 指定位置横坐标
 * @param y 指定位置纵坐标
 * @return uint16_t 返回指定位置的颜色
 */
uint16_t dispcolor_GetPixel(int16_t x, int16_t y)
{
#if (ST7789_MODE == ST7789_BUFFER_MODE)
    return st7789_GetPixel(x, y);
#else
    return 0x0000;
#endif
}

/**
 * @brief 填充矩形
 *
 * @param x 坐标
 * @param y 坐标
 * @param w 宽
 * @param h 高
 * @param color 颜色
 */
void dispcolor_FillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
{
    st7789_FillRect(x, y, w, h, color);
}

/**
 * @brief 把显存内容刷新到液晶屏上
 *
 */
void dispcolor_Update(void)
{
#if (ST7789_MODE == ST7789_BUFFER_MODE)
    st7789_update();
#endif
}

/**
 * @brief 设置全屏幕显示指定颜色
 *
 * @param color 用于显示的颜色
 */
void dispcolor_FillScreen(uint16_t color)
{
    dispcolor_FillRect(0, 0, dispcolor_getWidth(), dispcolor_getHeight(), color);
}

/**
 * @brief 全屏显示黑色
 *
 */
void dispcolor_ClearScreen(void)
{
    dispcolor_FillScreen(BLACK);
}

/**
 * @brief 在屏幕上绘制一条斜线
 *
 * @param x1
 * @param y1
 * @param x2
 * @param y2
 * @param color
 */
static void dispcolor_DrawLine_Slow(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color)
{
    const int16_t deltaX = abs(x2 - x1);
    const int16_t deltaY = abs(y2 - y1);
    const int16_t signX = x1 < x2 ? 1 : -1;
    const int16_t signY = y1 < y2 ? 1 : -1;

    int16_t error = deltaX - deltaY;

    dispcolor_DrawPixel(x2, y2, color);

    while (x1 != x2 || y1 != y2) {
        dispcolor_DrawPixel(x1, y1, color);
        const int16_t error2 = error * 2;

        if (error2 > -deltaY) {
            error -= deltaY;
            x1 += signX;
        }
        if (error2 < deltaX) {
            error += deltaX;
            y1 += signY;
        }
    }
}

/**
 * @brief 在显示屏上绘制一条直线
 *
 * @param x1
 * @param y1
 * @param x2
 * @param y2
 * @param color
 */
void dispcolor_DrawLine(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color)
{
    if (x1 == x2) {
        // 水平线 用快速方法画线
        if (y1 > y2) {
            dispcolor_FillRect(x1, y2, 1, y1 - y2 + 1, color);
        } else {
            dispcolor_FillRect(x1, y1, 1, y2 - y1 + 1, color);
        }

    } else if (y1 == y2) {
        // 垂直线 用快速方法画线
        if (x1 > x2) {
            dispcolor_FillRect(x2, y1, x1 - x2 + 1, 1, color);
        } else {
            dispcolor_FillRect(x1, y1, x2 - x1 + 1, 1, color);
        }

    } else {
        // 逐像素画线 斜线
        dispcolor_DrawLine_Slow(x1, y1, x2, y2, color);
    }
}

/**
 * @brief 在显示器上绘制一个矩形
 *
 * @param x1
 * @param y1
 * @param x2
 * @param y2
 * @param color
 */
void dispcolor_DrawRectangle(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color)
{
    dispcolor_DrawLine(x1, y1, x1, y2, color);
    dispcolor_DrawLine(x2, y1, x2, y2, color);
    dispcolor_DrawLine(x1, y1, x2, y1, color);
    dispcolor_DrawLine(x1, y2, x2, y2, color);
}

/**
 * @brief 在显示屏上绘制一个填充的矩形
 *
 * @param x1
 * @param y1
 * @param x2
 * @param y2
 * @param fillcolor
 */
void dispcolor_DrawRectangleFilled(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t fillcolor)
{
    if (x1 > x2)
        SwapInt16Values(&x1, &x2);
    if (y1 > y2)
        SwapInt16Values(&y1, &y2);

    dispcolor_FillRect(x1, y1, x2 - x1 + 1, y2 - y1 + 1, fillcolor);
}

/**
 * @brief 在显示屏上绘制一个圆圈
 *
 * @param x0 圆心的坐标
 * @param y0 圆心的坐标
 * @param radius 半径
 * @param color
 */
void dispcolor_DrawCircle(int16_t x0, int16_t y0, int16_t radius, uint16_t color)
{
    int x = 0;
    int y = radius;
    int delta = 1 - 2 * radius;
    int error = 0;

    while (y >= 0) {
        dispcolor_DrawPixel(x0 + x, y0 + y, color);
        dispcolor_DrawPixel(x0 + x, y0 - y, color);
        dispcolor_DrawPixel(x0 - x, y0 + y, color);
        dispcolor_DrawPixel(x0 - x, y0 - y, color);
        error = 2 * (delta + y) - 1;

        if (delta < 0 && error <= 0) {
            ++x;
            delta += 2 * x + 1;
            continue;
        }

        error = 2 * (delta - x) - 1;

        if (delta > 0 && error > 0) {
            --y;
            delta += 1 - 2 * y;
            continue;
        }

        ++x;
        delta += 2 * (x - y);
        --y;
    }
}

/**
 * @brief 在显示屏上绘制一个实心圆圈
 *
 * @param x0 圆心的坐标
 * @param y0 圆心的坐标
 * @param radius 半径
 * @param fillcolor
 */
void dispcolor_DrawCircleFilled(int16_t x0, int16_t y0, int16_t radius, uint16_t fillcolor)
{
    int x = 0;
    int y = radius;
    int delta = 1 - 2 * radius;
    int error = 0;

    while (y >= 0) {
        dispcolor_DrawLine(x0 + x, y0 - y, x0 + x, y0 + y, fillcolor);
        dispcolor_DrawLine(x0 - x, y0 - y, x0 - x, y0 + y, fillcolor);
        error = 2 * (delta + y) - 1;

        if (delta < 0 && error <= 0) {
            ++x;
            delta += 2 * x + 1;
            continue;
        }

        error = 2 * (delta - x) - 1;

        if (delta > 0 && error > 0) {
            --y;
            delta += 1 - 2 * y;
            continue;
        }

        ++x;
        delta += 2 * (x - y);
        --y;
    }
}

/**
 * @brief 在显示屏上显示 Char 字符
 * 这个函数如果超过了屏幕宽高，则不会显示了（不会自动换行处理）
 * @param X 起始坐标
 * @param Y 起始坐标
 * @param FontID 字体ID
 * @param Char 字符
 * @param TextColor 文本颜色
 * @param BgColor 背景颜色
 * @param TransparentBg 字符串背景是否透明
 * @return uint8_t 返回渲染字符的宽度
 */
static uint8_t dispcolor_DrawChar_General(int16_t X, int16_t Y, uint8_t FontID, uint8_t Char, uint16_t TextColor, uint16_t BgColor, uint8_t TransparentBg)
{
    // 指向特定字体字符的子标签的指针
    uint8_t* pCharTable = font_GetFontStruct(FontID, Char);
    if (NULL == pCharTable) {
        return 0;
    }

    uint8_t* pCharData = font_GetCharFont(pCharTable); // 点阵字符信息
    uint8_t CharWidth = font_GetCharWidth(pCharTable); // 符号宽度
    uint8_t CharHeight = font_GetCharHeight(pCharTable); // 符号高度

    if (FontID == FONTID_6X8M) {
        for (uint8_t row = 0; row < CharHeight; row++) {
            for (uint8_t col = 0; col < CharWidth; col++) {
                if (pCharData[row] & (1 << (7 - col))) {
                    dispcolor_DrawPixel(X + col, Y + row, TextColor); // 绘制文本
                } else if (!TransparentBg) {
                    dispcolor_DrawPixel(X + col, Y + row, BgColor); // 绘制文本背景色
                }
            }
        }

    } else {
        for (uint8_t row = 0; row < CharHeight; row++) {
            for (uint8_t col = 0; col < CharWidth; col++) {
                if (col < 8) {
                    if (pCharData[row * 2] & (1 << (7 - col))) {
                        dispcolor_DrawPixel(X + col, Y + row, TextColor);
                    } else if (!TransparentBg) {
                        dispcolor_DrawPixel(X + col, Y + row, BgColor);
                    }

                } else {
                    if (pCharData[(row * 2) + 1] & (1 << (15 - col))) {
                        dispcolor_DrawPixel(X + col, Y + row, TextColor);
                    } else if (!TransparentBg) {
                        dispcolor_DrawPixel(X + col, Y + row, BgColor);
                    }
                }
            }
        }
    }

    return CharWidth;
}

/**
 * @brief 在显示器上显示字符串 Str 中的文本的功能
 *
 * @param X 起始坐标
 * @param Y 起始坐标
 * @param FontID 字体ID
 * @param Str 字符串
 * @param TextColor 文本颜色
 * @param BgColor 文字背景颜色
 * @param TransparentBg 字符串背景是否透明
 * @return int16_t 下一个文字的开始显示X坐标
 */
static int16_t dispcolor_DrawString_General(int16_t X, int16_t Y, uint8_t FontID, uint8_t* Str, uint16_t TextColor, uint16_t BgColor, uint8_t TransparentBg)
{
    uint8_t done = 0; // 输出结束标志
    int16_t Xstart = X; // 起始显示坐标做为下一行行首
    uint8_t StrHeight = 8; // 移动到下一行的字符高度（以像素为单位）

    // 字符串循环显示
    while (!done) {
        switch (*Str) {
        case '\0':
            // 行结束
            done = 1;
            break;

        case '\n':
            // 移动到下一行
            Y += StrHeight;
            break;

        case '\r':
            // 移动到行首
            X = Xstart;
            break;

        default:
            // 显示字符
            if (TransparentBg)
                X += dispcolor_DrawChar_General(X, Y, FontID, *Str, TextColor, 0, 1); // 透明 每有背景色
            else
                X += dispcolor_DrawChar_General(X, Y, FontID, *Str, TextColor, BgColor, 0); // 不透明 有背景色

            StrHeight = font_GetCharHeight(font_GetFontStruct(FontID, *Str));
            break;
        }
        Str++;
    }
    return X;
}

/**
 * @brief 在显示器上显示字符串 Str 中的文本的功能
 *
 * @param X 起始坐标
 * @param Y 起始坐标
 * @param FontID 字体ID
 * @param Str 字符串指针
 * @param TextColor 文本颜色
 * @return int16_t 下一个文字的开始显示X坐标
 */
int16_t dispcolor_DrawString(int16_t X, int16_t Y, uint8_t FontID, uint8_t* Str, uint16_t TextColor)
{
    return dispcolor_DrawString_General(X, Y, FontID, Str, TextColor, 0, 1);
}

/**
 * @brief 在显示器上显示字符串 Str 中的文本的功能 带背景色显示
 *
 * @param X 起始坐标
 * @param Y 起始坐标
 * @param FontID 字体ID
 * @param Str 字符串
 * @param TextColor 颜色
 * @param BgColor 背景颜色
 * @return int16_t 下一个文字的开始显示X坐标
 */
int16_t dispcolor_DrawString_Bg(int16_t X, int16_t Y, uint8_t FontID, uint8_t* Str, uint16_t TextColor, uint16_t BgColor)
{
    return dispcolor_DrawString_General(X, Y, FontID, Str, TextColor, BgColor, 0);
}

/**
 * @brief 显示格式化的字符串
 *
 * @param X
 * @param Y
 * @param FontID
 * @param TextColor
 * @param args
 * @param ...
 * @return int16_t 下一个文字的开始显示X坐标
 */
int16_t dispcolor_printf(int16_t X, int16_t Y, uint8_t FontID, uint16_t TextColor, const char* args, ...)
{
    char StrBuff[256] = { 0 };

    va_list ap;
    va_start(ap, args);
    vsnprintf(StrBuff, sizeof(StrBuff), args, ap);
    va_end(ap);

    return dispcolor_DrawString(X, Y, FontID, (uint8_t*)StrBuff, TextColor);
}

/**
 * @brief 显示格式化的字符串 带背景色显示
 *
 * @param X
 * @param Y
 * @param FontID
 * @param TextColor
 * @param BgColor
 * @param args
 * @param ...
 * @return int16_t
 */
int16_t dispcolor_printf_Bg(int16_t X, int16_t Y, uint8_t FontID, uint16_t TextColor, uint16_t BgColor, const char* args, ...)
{
    char StrBuff[256] = { 0 };

    va_list ap;
    va_start(ap, args);
    vsnprintf(StrBuff, sizeof(StrBuff), args, ap);
    va_end(ap);

    return dispcolor_DrawString_Bg(X, Y, FontID, (uint8_t*)StrBuff, TextColor, BgColor);
}

/**
 * @brief 以像素为单位返回字符串的总宽度
 *
 * @param FontID
 * @param Str
 * @return int16_t
 */
int16_t dispcolor_getStrWidth(uint8_t FontID, char* Str)
{
    uint8_t done = 0; // 输出结束标志
    int16_t StrWidth = 0; // 线宽（以像素为单位）

    while (!done) {
        switch (*Str) {
        case '\0': // 行结束
            done = 1;
            break;
        case '\n': // 移动到下一行
        case '\r': // 移动到行首
            break;
        default: // 显示字符
            StrWidth += font_GetCharWidth(font_GetFontStruct(FontID, *Str));
            break;
        }
        Str++;
    }

    return StrWidth;
}

/**
 * @brief 以像素为单位返回格式化字符串的宽度
 *
 * @param FontID
 * @param args
 * @param ...
 * @return int16_t
 */
int16_t dispcolor_getFormatStrWidth(uint8_t FontID, const char* args, ...)
{
    char StrBuff[256] = { 0 };

    va_list ap;
    va_start(ap, args);
    vsnprintf(StrBuff, sizeof(StrBuff), args, ap);
    va_end(ap);

    return dispcolor_getStrWidth(FontID, StrBuff);
}

/**
 * @brief 将屏幕上图像的亮度降低了约 2 倍
 *
 */
void dispcolor_screenDark(void)
{
    for (uint16_t y = 0; y < dispcolor_getHeight(); y++) {
        for (uint16_t x = 0; x < dispcolor_getWidth(); x++) {
            uRGB565 color;
            color.value = dispcolor_GetPixel(x, y);
            color.rgb_color.r >>= 2;
            color.rgb_color.g >>= 2;
            color.rgb_color.b >>= 2;
            dispcolor_DrawPixel(x, y, color.value);
        }
    }
}

/**
 * @brief 复制显存数据到指定内存,保存位图的时候使用到
 *
 * @param pBuff
 */
void dispcolor_getScreenData(uint16_t* pBuff)
{
#if (ST7789_MODE == ST7789_BUFFER_MODE)
    st7789_getScreenData(pBuff);
#endif
}
