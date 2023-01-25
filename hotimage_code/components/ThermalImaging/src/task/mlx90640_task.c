#include "thermalimaging.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <string.h>

#define MLX_IIC_ADDRESS 0x33u
#define TA_SHIFT 8 // the default shift for a MLX90640 device in open air

static paramsMLX90640* pMLX90640params = NULL; // MLX90640 解析出的参数
sMlxData* pMlxData = NULL; // MLX90640 定义2个缓存
static int8_t lastFrameNo = 0;

const float FPS_RATES[] = { 0.5, 1, 2, 4, 8, 16, 32, 64 }; // MLX90640帧率
const int FPS_RATES_COUNT = sizeof(FPS_RATES) / sizeof(FPS_RATES[0]);

const int RESOLUTION[] = { 16, 17, 18, 19 }; // MLX90640 AD分辨率
const int RESOLUTION_COUNT = sizeof(RESOLUTION) / sizeof(RESOLUTION[0]);

static uint8_t MLX90640PausePlay = 0; // 暂停LCD刷新 继续LCD刷新功能

#ifndef max
#define max(a, b) ((a) > (b) ? (a) : (b))
#endif

/**
 * @brief 程序将温度矩阵复制到pbuff缓冲区
 *
 * @param pBuff
 */
void GetThermoData(float* pBuff)
{
    sMlxData* _pMlxData = &pMlxData[lastFrameNo];
    float* ThermoImage = _pMlxData->ThermoImage;
    memcpy(pBuff, ThermoImage, sizeof(_pMlxData->ThermoImage));
}

void GetThermoParams(paramsMLX90640* pBuf)
{
    if (pBuf) {
        memcpy(pBuf, pMLX90640params, sizeof(paramsMLX90640));
    }
}

/**
 * @brief 设置MLX90640是否粘贴
 *
 * @param isPause 1=暂停读取 0=开始读取
 */
uint8_t setMLX90640IsPause(uint8_t isPause)
{
    uint8_t last = MLX90640PausePlay;
    MLX90640PausePlay = isPause;
    return last;
}

/**
 * @brief 设置MLX90640 帧率
 *
 */
int mlx90640_flushRate(void)
{
    return MLX90640_SetRefreshRate(MLX_IIC_ADDRESS, settingsParms.MLX90640FPS);
}

/**
 * @brief 设置测量分辨率
 *
 * @return int
 */
int mlx90640_flushResolution(void)
{
    return MLX90640_SetResolution(MLX_IIC_ADDRESS, settingsParms.Resolution);
}

/**
 * @brief MLX90640线程
 *
 * @param arg
 */
void mlx90640_task(void* arg)
{
    int result;

    pMlxData = heap_caps_malloc(sizeof(sMlxData) << 1, MALLOC_CAP_8BIT);
    pMLX90640params = heap_caps_malloc(sizeof(paramsMLX90640), MALLOC_CAP_8BIT);
    uint16_t* pMLX90640Frame = heap_caps_malloc(max(MLX90640_getFrameSize(), MLX90640_getEEPROMSize()) << 1, MALLOC_CAP_8BIT); // 分配 MLX90640 使用的内存, EEPROM读取解析完后 数据就没用了

    if (!pMlxData || !pMLX90640params || !pMLX90640Frame) {
        FatalErrorMsg("Error allocating ram for MLX framebuffer %p %p %p\r\n", pMlxData, pMLX90640params, pMLX90640Frame);
        goto error;
    }

    // 初始化 mlx90640
    MLX90640_Init();

    vTaskDelay(100 / portTICK_RATE_MS);

    // 设定热成像帧率
    result = mlx90640_flushRate();
    if (result < 0) {
        goto error;
    }

    // 设定分辨率
    result = mlx90640_flushResolution();
    if (result < 0) {
        goto error;
    }

    // 设置棋盘模式
    result = MLX90640_SetChessMode(MLX_IIC_ADDRESS);
    if (result < 0) {
        goto error;
    }

    // 从EEPROM读取MLX90640参数
    result = MLX90640_DumpEE(MLX_IIC_ADDRESS, pMLX90640Frame);
    if (result < 0) {
        goto error;
    }

    // 解析EEPROM数据
    result = MLX90640_ExtractParameters(pMLX90640Frame, pMLX90640params);
    if (result < 0) {
        goto error;
    }

    // 等待一帧结束
    MLX90640_SynchFrame(MLX_IIC_ADDRESS);

    while (1) {
        if (0 == MLX90640PausePlay) {
            // 从传感器读取帧
            sMlxData* _pMlxData = &pMlxData[lastFrameNo];
            float* pThermoImage = _pMlxData->ThermoImage;

            // 连续读取帧 然后计算
            uint8_t idx = 0;
            while (true) {
                result = MLX90640_GetFrameData(MLX_IIC_ADDRESS, pMLX90640Frame);
                if ((0 == result || 1 == result) && (idx == result)) {
                    // 从MLX90640读取并输出多个参数
                    _pMlxData->Vdd = MLX90640_GetVdd(pMLX90640Frame, pMLX90640params); // 电压
                    _pMlxData->Ta = MLX90640_GetTa(pMLX90640Frame, pMLX90640params); // 实时外壳温度

                    // 计算环境温度用于温度补偿 手册上说的环境温度可以用外壳温度-8℃
                    float tr = _pMlxData->Ta - TA_SHIFT;

                    // 计算每个像素的温度
                    MLX90640_CalculateTo(pMLX90640Frame, pMLX90640params, settingsParms.Emissivity, tr, pThermoImage);

                    // 计算损坏的像素值
                    MLX90640_BadPixelsCorrection(pMLX90640params->brokenPixels, pThermoImage, 1, pMLX90640params);
                    MLX90640_BadPixelsCorrection(pMLX90640params->outlierPixels, pThermoImage, 1, pMLX90640params);

                    idx++;
                    if (idx >= 2) {
                        break;
                    }
                }
            }

            if (NULL != pHandleEventGroup)
                xEventGroupSetBits(pHandleEventGroup, 1 << lastFrameNo);
            lastFrameNo = (lastFrameNo + 1) & 1;

        } else {
            vTaskDelay(100 / portTICK_RATE_MS);
        }
    }

error:
    if (NULL != pMLX90640Frame) {
        heap_caps_free(pMLX90640Frame);
        pMLX90640Frame = NULL;
    }
    vTaskDelete(NULL);
    FatalErrorMsg("Error mlx90640 tasks\r\n");
}
