#include "console.h"
#include "dispcolor.h"
#include "esp_system.h"
#include "font.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdarg.h>
#include <string.h>

#define LineHeight 8 // 每行高度
static uint8_t ConsoleLine = 0; // 当前液晶第几行

/**
 * @brief 程序暂停控制台输出
 *
 * @param timeMs
 */
void console_pause(uint32_t timeMs)
{
    dispcolor_Update();
    vTaskDelay(timeMs / portTICK_RATE_MS);
}

/**
 * @brief 向控制台添加了一个新行
 *
 * @param msgType
 * @param args
 * @param ...
 */
void console_printf(eConsoleMsgType msgType, const char* args, ...)
{
    uint16_t TextColor = WHITE;
    char StrBuff[256] = { 0 };

    // 液晶屏幕上输出的颜色
    switch (msgType) {
    case MsgInfo:
        TextColor = GREEN;
        break;

    case MsgWarning:
        TextColor = RGB565(249, 166, 2);
        break;

    case MsgError:
        TextColor = RED;
        break;
    }

    va_list ap;
    va_start(ap, args);
    vsnprintf(StrBuff, sizeof(StrBuff), args, ap);
    va_end(ap);

    // 显示到液晶屏上
    dispcolor_DrawString(0, ConsoleLine * LineHeight, FONTID_6X8M, (uint8_t*)StrBuff, TextColor);

    // 从串口输出
    printf("%s", StrBuff);

    if (msgType != MsgInfo)
        console_pause(500);

    if (++ConsoleLine >= (dispcolor_getHeight() / LineHeight)) {
        ConsoleLine = 0;
        console_pause(300);
        dispcolor_ClearScreen();
    }
    dispcolor_Update();
}

/**
 * @brief 该过程将有关重新启动的消息打印到控制台并重新加载 esp32
 *
 */
static void FatalError()
{
    console_printf(MsgError, "Restart in 5 seconds...\r\n");
    vTaskDelay(5000 / portTICK_RATE_MS);

    fflush(stdout);
    esp_restart();
}

/**
 * @brief 该过程将错误和重启消息打印到控制台并重新加载 esp32
 *
 * @param args
 * @param ...
 */
void FatalErrorMsg(const char* args, ...)
{
    char StrBuff[256] = { 0 };

    va_list ap;
    va_start(ap, args);
    vsnprintf(StrBuff, sizeof(StrBuff), args, ap);
    va_end(ap);

    console_printf(MsgError, "%s\r\n", StrBuff);
    FatalError();
}
