#ifndef SETTINGS_H_
#define SETTINGS_H_

#include "esp_system.h"

// 原始热谱图的分辨率
#define THERMALIMAGE_RESOLUTION_WIDTH 32 // 宽
#define THERMALIMAGE_RESOLUTION_HEIGHT 24 // 高

//
#define MIN_TEMPSCALE_DELTA 10 // 最小温标增量
#define SCALE_DEFAULT_MIN 10 // 默认最小刻度温度
#define SCALE_DEFAULT_MAX 50 // 默认最大刻度温度

// 背光设置
#define BRIGHTNESS_STEP 5 // 背光每次递加递减数量
#define BRIGHTNESS_MIN 25 // 背光最低亮度
#define BRIGHTNESS_MAX 100 // 背光最高亮度

// 辐射率设置
#define EMISSIVITY_STEP 0.01
#define EMISSIVITY_MIN 0.01
#define EMISSIVITY_MAX 1

typedef enum {
    int8,
    uint8,
    int16,
    uint16,
    int32,
    uint32,
    int64,
    uint64,
    str,
    float32
} eType;

/**
 * @brief 按钮快捷功能
 *
 */
typedef enum {
    Emissivity_Plus, // 辐射率增加
    Emissivity_Minus, // 辐射率减小
    Scale_Next, //
    Scale_Prev,
    Markers_OnOff,
    Save_BMP16, // 保存BMP16
    Save_CSV, // 保存CSV
    Brightness_Plus, // 增加背光
    Brightness_Minus, // 减小背光
    Save_90640Params, // 保存 90640 参数表
    PausePlay, // 暂停\播放
} eButtonFunc;

// 图像插值算法
typedef enum {
    ORIGINAL = 0, // 原始 不插值
    LINEAR, // 线性插值
    HQ3X_2X, // 高斯模糊 双线性插值
} eScaleMode;

// 伪彩色类型
typedef enum {
    Iron = 0,
    Rainbow,
    Rainbow2,
    BlueRed,
    BlackNWhite,
    COLOR_MAX, // 放在最后
} eColorScale;

// 保存的设置
typedef struct
{
    float Emissivity; // 辐射率 是指被测物体的属性
    eScaleMode ScaleMode; // 插值算法
    uint8_t MLX90640FPS; // 刷新率
    uint8_t Resolution; // AD分辨率
    uint8_t AutoScaleMode; // 自动缩放模式
    float minTempNew; // 最小温度
    float maxTempNew; // 最大温度
    uint8_t TempMarkers; // 显示最大 最小温度标记
    eColorScale ColorScale; // 伪彩色
    int LcdBrightness; // LCD 背光亮度
    eButtonFunc FuncUp; // 按钮Up 类型
    eButtonFunc FuncCenter; // 按钮Center 类型
    eButtonFunc FuncDown; // 按钮Down 类型
} structSettingsParms;

extern structSettingsParms settingsParms;

int settings_storage_init(void);
int settings_read_all(void);
int settings_write_all(void);

int setting_read(char* pKey, eType type, void* pValue);
int setting_write(char* pKey, eType type, void* pValue);
int32_t settings_commit(void);

#endif /* SETTINGS_H_ */
