#ifndef MAIN_CONSOLE_CONSOLE_H_
#define MAIN_CONSOLE_CONSOLE_H_

#include "esp_system.h"

// 控制台消息类型
typedef enum {
    MsgInfo = 0,
    MsgWarning = 1,
    MsgError = 2
} eConsoleMsgType;

// 程序暂停控制台输出
void console_pause(uint32_t timeMs);
// 该过程向控制台添加了一个新行
void console_printf(eConsoleMsgType msgType, const char* args, ...);
// 该过程将错误和重启消息打印到控制台并重新加载 esp32
void FatalErrorMsg(const char* args, ...);

#endif /* MAIN_CONSOLE_CONSOLE_H_ */
