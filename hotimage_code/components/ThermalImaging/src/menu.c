#include "menu.h"
#include "CelsiusSymbol.h"
#include "dispcolor.h"
#include "palette.h"
#include "settings.h"
#include "spi_lcd.h"
#include "st7789.h"
#include "thermalimaging.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdio.h>
#include <string.h>

static sMenu Menu = { 0 }; // 菜单结构体

#define MENU_ITEMCOUNT_DEF 15 // 默认菜单条目个数
#define MENU_MALLOC_COUNT 5 // 每次分配空间个数

#define ItemFont FONTID_6X8M // 默认使用渲染字体
#define ItemLineHeight 12 // 每行使用多少个像素

#define MenuScaleLen 60
static tRGBcolor pPaletteMenu[MenuScaleLen];

static void menu_build();

extern void buildPalette(void);
extern int mlx90640_flushRate(void);
extern int mlx90640_flushResolution(void);

/**
 * @brief 初始化菜单
 *
 */
static void init_menu(void)
{
    if (0 == Menu.isInit) {
        Menu.defMemoryCount = MENU_ITEMCOUNT_DEF + 1;
        size_t malloc_size = Menu.defMemoryCount * sizeof(sMenuItem) * sizeof(uint8_t);

        // 分配稍微大一点的内存空间
        Menu.items = heap_caps_malloc(malloc_size, MALLOC_CAP_8BIT);
        if (NULL != Menu.items) {
            memset(Menu.items, 0, malloc_size);
        }

        Menu.itemsCount = 0;
        menu_build();

        // 计算菜单显示位置
        Menu.startX = 45;
        Menu.startY = (dispcolor_getHeight() - ((Menu.itemsCount + 1) * ItemLineHeight)) >> 1; // 每行占用12个像素

        // 菜单标题
        strcpy(Menu.Title, "MLX90640 Settings");

        // 初始化完成
        Menu.isInit = 1;
    }

    Menu.selectedItemIdx = 0;
    Menu.exitRequest = 0;
}

/**
 * @brief 退出菜单显示
 *
 */
static void menu_exit(void)
{
    Menu.exitRequest = 1;
}

/**
 * @brief 重新刷新右边的伪彩色比例尺
 *
 */
static void MenuAction_ReBuildPalette()
{
    buildPalette();
}

/**
 * @brief 菜单确定 修改LCD背光
 *
 */
static void MenuAction_LCDBrightnes()
{
    dispcolor_SetBrightness(settingsParms.LcdBrightness);
}

/**
 * @brief 设置MLX90640帧率
 *
 */
static void MenuAction_MLX90640Rate()
{
    mlx90640_flushRate();
}

/**
 * @brief 设置MLX90640 AD分辨率
 *
 */
static void MenuAction_MLX90640Resolution()
{
    mlx90640_flushResolution();
}

/**
 * @brief 添加菜单条目
 *
 * @param pNewItem 添加的子菜单条目
 */
static void add_menuitem(sMenuItem* pNewItem)
{
    // 超出菜单个数了 分配新内存
    if ((Menu.itemsCount + 1) > (Menu.defMemoryCount - 1)) {
        sMenuItem* pMenuArray = heap_caps_malloc((Menu.defMemoryCount + MENU_MALLOC_COUNT) * sizeof(sMenuItem) * sizeof(uint8_t), MALLOC_CAP_8BIT);
        if (NULL != pMenuArray) {
            // 复制原来数据到新内存
            memcpy(pMenuArray, Menu.items, Menu.defMemoryCount * sizeof(sMenuItem) * sizeof(uint8_t));

            // 新内存数量增加
            Menu.defMemoryCount += MENU_MALLOC_COUNT;

            // 释放之前分配的内存 并 设置新内存地址
            heap_caps_free(Menu.items);
            Menu.items = pMenuArray;
        }
    }

    // 复制
    memcpy(&(Menu.items[Menu.itemsCount]), pNewItem, sizeof(sMenuItem));
    Menu.itemsCount++;
}

/**
 * @brief 构建菜单
 *
 * @return void
 */
static void menu_build()
{
    sMenuItem item; // 菜单条目
    uint8_t idx;

    // 物体反射率
    strcpy(item.Title, "Emissivity:");
    item.ItemType = FloatValue;
    item.pValue = &settingsParms.Emissivity;
    item.Min = EMISSIVITY_MIN;
    item.Max = EMISSIVITY_MAX;
    strcpy(item.Unit, "");
    item.EditStep = EMISSIVITY_STEP;
    item.DigitsAfterPoint = 2;
    item.EnterAction = NULL;
    item.Action = NULL;
    add_menuitem(&item);

    // 插值算法
    strcpy(item.Title, "Interpolation:");
    item.ItemType = ComboBox;
    item.ComboItemsCount = 3;
    item.ComboItems = heap_caps_malloc(item.ComboItemsCount * sizeof(sComboItem), MALLOC_CAP_8BIT);
    strcpy(item.ComboItems[0].Str, "Original"); // 原始
    strcpy(item.ComboItems[1].Str, "Linear"); // 线性
    strcpy(item.ComboItems[2].Str, "Gauss + Bilinear"); // 高斯
    item.pValue = &settingsParms.ScaleMode;
    item.EnterAction = NULL;
    item.Action = NULL;
    add_menuitem(&item);

    // MLX90640 刷新率
    strcpy(item.Title, "Frequency:");
    item.ItemType = ComboBox;
    item.ComboItemsCount = FPS_RATES_COUNT;
    item.ComboItems = heap_caps_malloc(item.ComboItemsCount * sizeof(sComboItem), MALLOC_CAP_8BIT);
    for (int k = 0; k < item.ComboItemsCount; k++) {
        sprintf(item.ComboItems[k].Str, "%.1f FPS", FPS_RATES[k]);
    }
    item.pValue = &settingsParms.MLX90640FPS;
    item.EnterAction = MenuAction_MLX90640Rate;
    item.Action = NULL;
    add_menuitem(&item);

    // MLX90640 分辨率
    strcpy(item.Title, "Resolution:");
    item.ItemType = ComboBox;
    item.ComboItemsCount = RESOLUTION_COUNT;
    item.ComboItems = heap_caps_malloc(item.ComboItemsCount * sizeof(sComboItem), MALLOC_CAP_8BIT);
    for (int k = 0; k < item.ComboItemsCount; k++) {
        sprintf(item.ComboItems[k].Str, "AD %d", RESOLUTION[k]);
    }
    item.pValue = &settingsParms.Resolution;
    item.EnterAction = MenuAction_MLX90640Resolution;
    item.Action = NULL;
    add_menuitem(&item);

    //
    strcpy(item.Title, "Auto Scaling:");
    item.ItemType = CheckBox;
    item.pValue = &settingsParms.AutoScaleMode;
    item.EnterAction = MenuAction_ReBuildPalette;
    item.Action = NULL;
    add_menuitem(&item);

    // 最大温度
    strcpy(item.Title, "MAX. Temp:");
    item.ItemType = FloatValue;
    item.pValue = &settingsParms.maxTempNew;
    item.Min = -40;
    item.Max = 300;
    strcpy(item.Unit, CELSIUS_SYMBOL);
    item.EditStep = 1;
    item.DigitsAfterPoint = 0;
    item.EnterAction = MenuAction_ReBuildPalette;
    item.Action = NULL;
    add_menuitem(&item);

    // 最小温度
    strcpy(item.Title, "MIN. Temp:");
    item.ItemType = FloatValue;
    item.pValue = &settingsParms.minTempNew;
    item.Min = -40;
    item.Max = 300;
    strcpy(item.Unit, CELSIUS_SYMBOL);
    item.EditStep = 1;
    item.DigitsAfterPoint = 0;
    item.EnterAction = MenuAction_ReBuildPalette;
    item.Action = NULL;
    add_menuitem(&item);

    // 显示/不显示 最大最小标记
    strcpy(item.Title, "MAX/MIN Mark:");
    item.ItemType = CheckBox;
    item.pValue = &settingsParms.TempMarkers;
    item.EnterAction = NULL;
    item.Action = NULL;
    add_menuitem(&item);

    // 伪彩色
    strcpy(item.Title, "Palette:");
    item.ItemType = PaletteBox; // ComboBox;
    item.ComboItemsCount = COLOR_MAX;
    item.ComboItems = heap_caps_malloc(item.ComboItemsCount * sizeof(sComboItem), MALLOC_CAP_8BIT);
    idx = 0;
    strcpy(item.ComboItems[idx++].Str, "Iron");
    strcpy(item.ComboItems[idx++].Str, "Rainbow");
    strcpy(item.ComboItems[idx++].Str, "Rainbow2");
    strcpy(item.ComboItems[idx++].Str, "BlueRed");
    strcpy(item.ComboItems[idx++].Str, "Black&White");
    item.pValue = &settingsParms.ColorScale;
    item.EnterAction = MenuAction_ReBuildPalette;
    item.Action = NULL;
    add_menuitem(&item);

#ifdef LCD_PIN_NUM_BCKL
    // 背光亮度
    strcpy(item.Title, "LCD Brightnes:");
    item.ItemType = IntValue;
    item.pValue = &settingsParms.LcdBrightness;
    item.Min = BRIGHTNESS_MIN;
    item.Max = BRIGHTNESS_MAX;
    strcpy(item.Unit, " %");
    item.EditStep = BRIGHTNESS_STEP;
    item.EnterAction = MenuAction_LCDBrightnes;
    item.Action = NULL;
    add_menuitem(&item);
#endif

    // 按钮设置
    strcpy(item.Title, "Up Button:");
    item.ItemType = ComboBox;
    item.ComboItemsCount = 9;
#ifdef LCD_PIN_NUM_BCKL
    item.ComboItemsCount += 2;
#endif
    item.ComboItems = heap_caps_malloc(item.ComboItemsCount * sizeof(sComboItem), MALLOC_CAP_8BIT);
    idx = 0;
    strcpy(item.ComboItems[idx++].Str, "Emissivity +");
    strcpy(item.ComboItems[idx++].Str, "Emissivity -");
    strcpy(item.ComboItems[idx++].Str, "Palette +");
    strcpy(item.ComboItems[idx++].Str, "Palette -");
    strcpy(item.ComboItems[idx++].Str, "MAX/MIN Mark");
    strcpy(item.ComboItems[idx++].Str, "Save BMP");
    strcpy(item.ComboItems[idx++].Str, "Save CSV");
#ifdef LCD_PIN_NUM_BCKL
    strcpy(item.ComboItems[idx++].Str, "Brightnes +");
    strcpy(item.ComboItems[idx++].Str, "Brightnes -");
#endif
    strcpy(item.ComboItems[idx++].Str, "MLX90640 Params");
    strcpy(item.ComboItems[idx++].Str, "Pause / Play");
    item.pValue = &settingsParms.FuncUp;
    item.EnterAction = NULL;
    item.Action = NULL;
    add_menuitem(&item);

    // 按钮设置
    strcpy(item.Title, "Center Button:");
    item.pValue = &settingsParms.FuncCenter;
    item.EnterAction = NULL;
    item.Action = NULL;
    add_menuitem(&item);

    // 按钮设置
    strcpy(item.Title, "Down Button:");
    item.pValue = &settingsParms.FuncDown;
    item.EnterAction = NULL;
    item.Action = NULL;
    add_menuitem(&item);

    // 保存 退出Menu
    strcpy(item.Title, "Save");
    item.ItemType = Button;
    item.EnterAction = NULL;
    item.Action = menu_exit;
    add_menuitem(&item);
}

/**
 * @brief 处理按键事件
 *
 */
static void menu_process_events(void)
{
    sMenuItem* item = &Menu.items[Menu.selectedItemIdx]; // 当前选择的条目

    uint8_t* pVal_uint8 = (uint8_t*)item->pValue;
    int* pVal_int = (int*)item->pValue;
    float* pVal_float = (float*)item->pValue;

    if (pHandleEventGroup) {
        EventBits_t uxBitsToWaitFor = RENDER_ShortPress_Up | RENDER_Hold_Up | RENDER_ShortPress_Center | RENDER_Hold_Center | RENDER_ShortPress_Down | RENDER_Hold_Down;
        EventBits_t bits = xEventGroupWaitBits(pHandleEventGroup, uxBitsToWaitFor, pdFALSE, pdFALSE, portMAX_DELAY);
        xEventGroupClearBits(pHandleEventGroup, bits);

        if ((bits & RENDER_ShortPress_Up) == RENDER_ShortPress_Up) {
            // 按钮 Up
            if (Menu.OnEdit) {
                // 编辑所选菜单的值
                switch (item->ItemType) {
                case CheckBox:
                    *pVal_uint8 = (*pVal_uint8 + 1) & 1;
                    break;

                case FloatValue:
                    *pVal_float += item->EditStep;
                    if (*pVal_float > item->Max)
                        *pVal_float = item->Max;
                    break;

                case IntValue:
                    *pVal_int += item->EditStep;
                    if (*pVal_int > item->Max)
                        *pVal_int = item->Max;
                    break;

                case PaletteBox:
                case ComboBox:
                    if (*pVal_uint8 == item->ComboItemsCount - 1)
                        *pVal_uint8 = 0;
                    else
                        *pVal_uint8 = *pVal_uint8 + 1;
                    break;

                default:
                    break;
                }

            } else {
                // 导航到上一个菜单
                if (0 != Menu.selectedItemIdx)
                    Menu.selectedItemIdx--;
                else
                    Menu.selectedItemIdx = Menu.itemsCount - 1;
            }
        }

        if ((bits & RENDER_ShortPress_Center) == RENDER_ShortPress_Center) {
            // 按钮 Center
            if (item->Action) {
                item->Action();

            } else {
                if (Menu.OnEdit) {
                    if (item->EnterAction) {
                        item->EnterAction();
                    }
                }
                Menu.OnEdit = !Menu.OnEdit; // 进入编辑 取消编辑
            }
        }

        if ((bits & RENDER_ShortPress_Down) == RENDER_ShortPress_Down) {
            // 按钮 Down
            if (Menu.OnEdit) {
                // 编辑所选菜单的值
                switch (item->ItemType) {
                case CheckBox:
                    *pVal_uint8 = (*pVal_uint8 + 1) & 1;
                    break;

                case FloatValue:
                    *pVal_float -= item->EditStep;
                    if (*pVal_float < item->Min)
                        *pVal_float = item->Min;
                    break;

                case IntValue:
                    *pVal_int -= item->EditStep;
                    if (*pVal_int < item->Min)
                        *pVal_int = item->Min;
                    break;

                case PaletteBox:
                case ComboBox:
                    if (*pVal_uint8 == 0)
                        *pVal_uint8 = item->ComboItemsCount - 1;
                    else
                        *pVal_uint8 = *pVal_uint8 - 1;
                    break;

                default:
                    break;
                }

            } else {
                // 导航到下一个菜单
                if (Menu.selectedItemIdx < (Menu.itemsCount - 1))
                    Menu.selectedItemIdx++;
                else
                    Menu.selectedItemIdx = 0;
            }
        }
    }
}

/**
 * @brief 菜单渲染
 *
 * @return int
 */
static int menu_render(void)
{
    // 渲染LCD上的坐标
    uint16_t startX = Menu.startX;
    uint16_t startY = Menu.startY;
    uint16_t endX = dispcolor_getWidth() - startX;
    uint16_t endY = dispcolor_getHeight() - startY;

    // 退出菜单渲染
    if (Menu.exitRequest)
        return 1;

    // 处理按键事件
    menu_process_events();

    // 在屏幕上绘制填充矩形 黑底
    dispcolor_DrawRectangleFilled(startX, startY, endX, endY, BLACK);

    // 显示菜单标题 白底
    dispcolor_DrawRectangleFilled(startX + 2, startY + 2, endX - 2, startY + ItemLineHeight, WHITE);
    dispcolor_printf(110, startY + 4, ItemFont, BLACK, Menu.Title);

    // 菜单内容起始坐标
    int yPos = ItemLineHeight;

    // 显示菜单项目
    if (Menu.items) {
        for (int i = 0; i < Menu.itemsCount; i++) {
            int16_t textWidth;
            uint16_t mainColorTitle, bgColorTitle, mainColorValue, bgColorValue;
            sMenuItem* item = &Menu.items[i];

            if (Menu.OnEdit) {
                // 编辑模式中
                mainColorTitle = WHITE;
                bgColorTitle = BLACK;
                mainColorValue = i == Menu.selectedItemIdx ? BLACK : WHITE;
                bgColorValue = i == Menu.selectedItemIdx ? RGB565(10, 200, 100) : BLACK;
                dispcolor_DrawRectangleFilled(endX - 100, startY + yPos + 2, endX - 2, startY + yPos + ItemLineHeight, bgColorValue);
                dispcolor_printf(startX + 7, startY + yPos + 4, ItemFont, mainColorTitle, item->Title);

            } else {
                // 不在编辑模式中
                mainColorTitle = i == Menu.selectedItemIdx ? BLACK : WHITE;
                bgColorTitle = i == Menu.selectedItemIdx ? RGB565(10, 200, 100) : BLACK;
                mainColorValue = WHITE;
                bgColorValue = BLACK;
                dispcolor_DrawRectangleFilled(startX + 2, startY + yPos + 2, endX - 100, startY + yPos + ItemLineHeight, bgColorTitle);
                dispcolor_printf(startX + 7, startY + yPos + 4, ItemFont, mainColorTitle, item->Title);
            }

            uint16_t color;

            // 带单类型
            switch (item->ItemType) {
            case CheckBox:
                if (*((uint8_t*)(item->pValue)))
                    dispcolor_printf(endX - dispcolor_getStrWidth(ItemFont, "ON") - 5, startY + yPos + 4, ItemFont, mainColorValue, "ON");
                else
                    dispcolor_printf(endX - dispcolor_getStrWidth(ItemFont, "OFF") - 5, startY + yPos + 4, ItemFont, mainColorValue, "OFF");
                break;

            case ComboBox:
                if (item->ComboItemsCount) {
                    char* pStr = item->ComboItems[*((uint8_t*)item->pValue)].Str;
                    textWidth = dispcolor_getStrWidth(ItemFont, pStr);
                    dispcolor_printf(endX - textWidth - 5, startY + yPos + 4, FONTID_6X8M, mainColorValue, pStr);
                }
                break;

            case PaletteBox:
                getPalette(settingsParms.ColorScale, MenuScaleLen, pPaletteMenu);
                for (int i = 0; i < MenuScaleLen; i++) {
                    color = RGB565(pPaletteMenu[i].r, pPaletteMenu[i].g, pPaletteMenu[i].b);
                    dispcolor_FillRect(endX - MenuScaleLen + i - 6, startY + yPos + 3, 1, 8, color);
                }
                dispcolor_DrawRectangle(endX - MenuScaleLen - 6, startY + yPos + 3, endX - 6, startY + yPos + 11, WHITE);
                break;

            case FloatValue:
                switch (item->DigitsAfterPoint) {
                case 0:
                    textWidth = dispcolor_getFormatStrWidth(ItemFont, "%.0f%s", *((float*)(item->pValue)), item->Unit);
                    dispcolor_printf(endX - textWidth - 5, startY + yPos + 4, ItemFont, mainColorValue, "%.0f%s", *((float*)(item->pValue)), item->Unit);
                    break;
                case 1:
                    textWidth = dispcolor_getFormatStrWidth(ItemFont, "%.1f%s", *((float*)(item->pValue)), item->Unit);
                    dispcolor_printf(endX - textWidth - 5, startY + yPos + 4, ItemFont, mainColorValue, "%.1f%s", *((float*)(item->pValue)), item->Unit);
                    break;
                case 2:
                    textWidth = dispcolor_getFormatStrWidth(ItemFont, "%.2f%s", *((float*)(item->pValue)), item->Unit);
                    dispcolor_printf(endX - textWidth - 5, startY + yPos + 4, ItemFont, mainColorValue, "%.2f%s", *((float*)(item->pValue)), item->Unit);
                    break;
                case 3:
                    textWidth = dispcolor_getFormatStrWidth(ItemFont, "%.3f%s", *((float*)(item->pValue)), item->Unit);
                    dispcolor_printf(endX - textWidth - 5, startY + yPos + 4, ItemFont, mainColorValue, "%.3f%s", *((float*)(item->pValue)), item->Unit);
                    break;
                }
                break;

            case IntValue:
                textWidth = dispcolor_getFormatStrWidth(ItemFont, "%d%s", *((int*)(item->pValue)), item->Unit);
                dispcolor_printf(endX - textWidth - 5, startY + yPos + 4, ItemFont, mainColorValue, "%d%s", *((int*)(item->pValue)), item->Unit);
                break;

            default:
                break;
            }

            // 每行12像素
            yPos += ItemLineHeight;
        }
    }

    // 刷新LCD
    dispcolor_Update();

    return 0;
}

/**
 * @brief 运行菜单，显示菜单内容
 *
 * @return int
 */
int menu_run()
{
    setMLX90640IsPause(1);

    init_menu();

    // 降低后面像素的亮度
    dispcolor_screenDark();

    while (1) {
        // 渲染菜单
        if (menu_render())
            break;

        vTaskDelay((1000 / 30) / portTICK_RATE_MS);
    }

    setMLX90640IsPause(0);
    return 0;
}
