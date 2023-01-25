#include "messagebox.h"
#include "dispcolor.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <string.h>

/**
 * @brief 在屏幕中央显示一条消息
 *
 * @param width 弹窗宽度
 * @param ItemFont 字体
 * @param pTitle 标题内容
 * @param pMessage 消息内容
 * @param lineColor 线颜色
 * @param darkBg 使背景变暗
 * @param delay_ms
 */
void message_show(uint16_t width, uint8_t ItemFont, char* pTitle, char* pMessage, uint16_t lineColor, uint8_t darkBg, uint16_t delay_ms)
{
    // 计算所需的窗口宽度
    uint16_t titleWidth = dispcolor_getStrWidth(ItemFont, pTitle); // 标题宽度
    uint16_t messageWidth = dispcolor_getStrWidth(ItemFont, pMessage); // 消息内容宽度
    uint16_t windowWidth = width ? width : titleWidth > messageWidth ? titleWidth + 20
                                                                     : messageWidth + 10;
    uint16_t windowHeight = 40;

    // 我们计算窗口的坐标
    uint16_t lcd_width = dispcolor_getWidth();
    uint16_t startX = (lcd_width - windowWidth) / 2;
    uint16_t startY = 100;
    uint16_t endX = lcd_width - startX;
    uint16_t endY = startY + windowHeight;

    // 使背景变暗
    if (darkBg)
        dispcolor_screenDark();

    // 用黑色填充窗口
    dispcolor_DrawRectangleFilled(startX, startY, endX, endY, BLACK);

    // 显示窗口元素
    uint16_t yPos = 0;

    // 标题
    dispcolor_DrawRectangleFilled(startX + 2, startY + yPos + 2, endX - 2, startY + yPos + 12, WHITE);
    dispcolor_printf(startX + (windowWidth - titleWidth) / 2, startY + yPos + 4, FONTID_6X8M, BLACK, pTitle);
    yPos += 16;

    // 消息文本
    dispcolor_printf(startX + 5, startY + yPos + 4, FONTID_6X8M, WHITE, pMessage);
    yPos += 16;

    // 底部有彩色条纹
    dispcolor_DrawRectangleFilled(startX + 2, startY + yPos + 2, endX - 2, startY + yPos + 5, lineColor);

    dispcolor_Update();
    vTaskDelay(delay_ms / portTICK_RATE_MS);
}

/**
 * @brief 显示一个带有消息和颜色进度条的窗口
 *
 * @param width
 * @param ItemFont
 * @param pTitle
 * @param pMessage
 * @param progressColor 进度条颜色
 * @param progressValue 进度条当前值
 * @param progressMax 进度条最大值日
 */
void progress_show(uint16_t width, uint8_t ItemFont, char* pTitle, char* pMessage, uint16_t progressColor, uint16_t progressValue, uint16_t progressMax)
{
    // 计算所需的窗口宽度
    uint16_t titleWidth = dispcolor_getStrWidth(ItemFont, pTitle);
    uint16_t messageWidth = dispcolor_getStrWidth(ItemFont, pMessage);
    uint16_t windowWidth = width ? width : titleWidth > messageWidth ? titleWidth + 20
                                                                     : messageWidth + 10;
    uint16_t windowHeight = 40;

    // 我们计算窗口的坐标
    uint16_t lcd_width = dispcolor_getWidth();
    uint16_t startX = (lcd_width - windowWidth) / 2;
    uint16_t startY = 100;
    uint16_t endX = lcd_width - startX;
    uint16_t endY = startY + windowHeight;

    // 用黑色填充窗口
    dispcolor_DrawRectangleFilled(startX, startY, endX, endY, BLACK);

    // 显示窗口元素
    uint16_t yPos = 0;

    // 标题
    dispcolor_DrawRectangleFilled(startX + 2, startY + yPos + 2, endX - 2, startY + yPos + 12, WHITE);
    dispcolor_printf(startX + (windowWidth - titleWidth) / 2, startY + yPos + 4, FONTID_6X8M, BLACK, pTitle);
    yPos += 16;

    // 消息文本
    dispcolor_printf(startX + 5, startY + yPos + 4, FONTID_6X8M, WHITE, pMessage);
    yPos += 16;

    // 底部有彩色条纹
    if (progressMax) {
        // 计算出显示的百分比
        uint32_t endLineX = windowWidth - 4;
        endLineX = endLineX * progressValue / progressMax;
        dispcolor_DrawRectangleFilled(startX + 2, startY + yPos + 2, startX + endLineX, startY + yPos + 5, progressColor);
    }

    dispcolor_Update();
}

/**
 * @brief 该过程显示一个消息框
 *
 * @param width
 * @param ItemFont
 * @param pTitle
 * @param pMessage
 * @param progressColor
 * @param progressValue
 * @param progressMax
 */
void progress_start_show(uint16_t width, uint8_t ItemFont, char* pTitle, char* pMessage, uint16_t progressColor, uint16_t progressValue, uint16_t progressMax)
{
    // 使背景变暗
    dispcolor_screenDark();
    progress_show(width, ItemFont, pTitle, pMessage, progressColor, progressValue, progressMax);
}
