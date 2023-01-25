#ifndef MAIN_TASK_UI_H_
#define MAIN_TASK_UI_H_

#include "esp_system.h"
#include <freertos/event_groups.h>
#include <freertos/semphr.h>

extern EventGroupHandle_t* pHandleEventGroup;
extern SemaphoreHandle_t pSPIMutex;

typedef enum _render_type {
    RENDER_MLX90640_NO0 = 1 << 0, // 第0帧
    RENDER_MLX90640_NO1 = 1 << 1, // 第1帧
    RENDER_ShortPress_Up = 1 << 2, // Up按钮
    RENDER_Hold_Up = 1 << 3, // Up按钮长按
    RENDER_ShortPress_Center = 1 << 4, // Center按钮
    RENDER_Hold_Center = 1 << 5, // Center按钮长按
    RENDER_ShortPress_Down = 1 << 6, // Down按钮
    RENDER_Hold_Down = 1 << 7, // Down按钮长按
} render_type;

void render_task(void* arg);

void tips_printf(const char* args, ...);

#endif /* MAIN_TASK_UI_H_ */
