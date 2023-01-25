#ifndef MAIN_MENU_H_
#define MAIN_MENU_H_

#include "esp_system.h"

// 菜单类型
typedef enum {
    Button, // 按钮
    CheckBox, // 复选框
    FloatValue, // 浮点数值
    IntValue, // 数值
    ComboBox, // 组合框
    PaletteBox // 伪彩色
} eMenuItemType;

typedef struct {
    char Str[32];
} sComboItem;

// 菜单条目
typedef struct {
    char Title[40]; // 菜单条目标题
    eMenuItemType ItemType; // 菜单类型

    // Variable numerical value parameters (IntValue, FloatValue)
    float Min; // 最小
    float Max; // 最大
    float EditStep; // 步进
    uint8_t DigitsAfterPoint;
    char Unit[10];

    // Selectable item lines (in ComboBox)
    sComboItem* ComboItems; // 复选框
    uint8_t ComboItemsCount;

    // Variable value
    void* pValue; // Pointer to the value to be changed (for IntValue, FloatValue, ComboBox)

    // 菜单功能回调函数
    void (*EnterAction)(void);
    void (*Action)(void); // Pointer to the function of menu items that imply an action (e.g. Button)
} sMenuItem;

// 菜单结构体
typedef struct {
    uint16_t startX; // 显示位置
    uint16_t startY; // 显示位置
    uint16_t width; // 显示宽度

    char Title[30]; // 菜单标题

    sMenuItem* items; // 菜单具体条目
    uint8_t itemsCount; // 菜单个数
    uint8_t defMemoryCount; // 默认分配多少个菜单

    uint8_t selectedItemIdx; // 当前选择的条目
    uint8_t OnEdit; // 进入 退出 编辑模式
    uint8_t isInit; // 是否初始化
    uint8_t exitRequest; // 需要关闭菜单的标志
} sMenu;

int menu_run();

#endif /* MAIN_MENU_H_ */
