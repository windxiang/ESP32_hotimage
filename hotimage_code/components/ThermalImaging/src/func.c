#include "func.h"
#include "dispcolor.h"
#include "save.h"
#include "settings.h"
#include "thermalimaging.h"

typedef enum {
    FUNC_PLUS,
    FUNC_MINUS,
} FUNC_TYPES;

extern void buildPalette(void);

/**
 * @brief 修改 辐射率
 *
 */
static void Func_Emissivity(FUNC_TYPES type)
{
    switch (type) {
    case FUNC_PLUS: {
        if (settingsParms.Emissivity < EMISSIVITY_MAX)
            settingsParms.Emissivity += EMISSIVITY_STEP;
    } break;

    case FUNC_MINUS: {
        if (settingsParms.Emissivity > EMISSIVITY_MIN)
            settingsParms.Emissivity -= EMISSIVITY_STEP;
    } break;
    }

    setting_write("Emissivity", float32, &settingsParms.Emissivity);
    settings_commit();
}

/**
 * @brief 修改 伪彩色
 *
 */
static void Func_Scale(FUNC_TYPES type)
{
    switch (type) {
    case FUNC_PLUS: {
        if (settingsParms.ColorScale == (COLOR_MAX - 1))
            settingsParms.ColorScale = COLOR_MAX - 1;
        else
            settingsParms.ColorScale++;
    } break;

    case FUNC_MINUS: {
        if (0 == settingsParms.ColorScale)
            settingsParms.ColorScale = COLOR_MAX - 1;
        else
            settingsParms.ColorScale--;
    } break;
    }

    setting_write("ColorScale", uint8, &settingsParms.ColorScale);
    settings_commit();

    buildPalette();
}

/**
 * @brief 修改 LCD背光
 *
 */
static void Func_Brightness(FUNC_TYPES type)
{
    switch (type) {
    case FUNC_PLUS: {
        if (settingsParms.LcdBrightness < BRIGHTNESS_MAX)
            settingsParms.LcdBrightness += BRIGHTNESS_STEP;
    } break;

    case FUNC_MINUS: {
        if (settingsParms.LcdBrightness > BRIGHTNESS_MIN)
            settingsParms.LcdBrightness -= BRIGHTNESS_STEP;
    } break;
    }

    dispcolor_SetBrightness(settingsParms.LcdBrightness);

    setting_write("LcdBrightness", int32, &settingsParms.LcdBrightness);
    settings_commit();
}

/**
 * @brief 是否显示最大最小温度点
 *
 */
static void Func_TempMarkers(void)
{
    settingsParms.TempMarkers = (settingsParms.TempMarkers + 1) & 1;

    setting_write("TempMarkers", uint8, &settingsParms.TempMarkers);
    settings_commit();
}

/**
 * @brief 执行按钮功能
 *
 * @param btnSetup
 */
static void shortPressButtonHandler(eButtonFunc btnSetup)
{
    switch (btnSetup) {
    case Emissivity_Plus:
        Func_Emissivity(FUNC_PLUS);
        break;

    case Emissivity_Minus:
        Func_Emissivity(FUNC_MINUS);
        break;

    case Scale_Next:
        Func_Scale(FUNC_PLUS);
        break;

    case Scale_Prev:
        Func_Scale(FUNC_MINUS);
        break;

    case Brightness_Plus:
        Func_Brightness(FUNC_PLUS);
        break;

    case Brightness_Minus:
        Func_Brightness(FUNC_MINUS);
        break;

    case Markers_OnOff:
        Func_TempMarkers();
        break;

    case Save_BMP16:
        setMLX90640IsPause(1);
        save_ImageBMP(24);
        setMLX90640IsPause(0);
        break;

    case Save_CSV:
        setMLX90640IsPause(1);
        save_ImageCSV();
        setMLX90640IsPause(0);
        break;

    case Save_90640Params:
        setMLX90640IsPause(1);
        save_MLX90640Params();
        setMLX90640IsPause(0);
        break;

    case PausePlay: {
        uint8_t last = setMLX90640IsPause(true);
        setMLX90640IsPause((last + 1) & 1);
    } break;
    }
}

/**
 * @brief Up按钮
 *
 */
void FuncUp_Run(void)
{
    shortPressButtonHandler(settingsParms.FuncUp);
}

/**
 * @brief Center按钮
 *
 */
void FuncCenter_Run(void)
{
    shortPressButtonHandler(settingsParms.FuncCenter);
}

/**
 * @brief Down按钮
 *
 */
void FuncDown_Run(void)
{
    shortPressButtonHandler(settingsParms.FuncDown);
}
