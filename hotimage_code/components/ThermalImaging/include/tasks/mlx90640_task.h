#ifndef _MLX90640_TASK_H_
#define _MLX90640_TASK_H_

#include "esp_system.h"
#include "settings.h"

typedef struct
{
    float ThermoImage[THERMALIMAGE_RESOLUTION_WIDTH * THERMALIMAGE_RESOLUTION_HEIGHT]; // 热成像每个像素的温度值 32 x 24 = 768 像素 
    float Vdd;
    float Ta; // 环境温度 传感器周围的温度
	float CenterTemp; // 中心温度

	float minT; // 最小温度
	float maxT; // 最大温度
	int8_t minT_X; // 最小温度 坐标
	int8_t minT_Y;
	int8_t maxT_X; // 最大温度 坐标
	int8_t maxT_Y;
} sMlxData;

// MLX90640 最大最小温度
#define MIN_TEMP -40
#define MAX_TEMP 300

extern const float FPS_RATES[];
extern const int FPS_RATES_COUNT;

extern const int RESOLUTION[];
extern const int RESOLUTION_COUNT;

extern sMlxData* pMlxData;


// 该过程将温度矩阵复制到 pBuff 缓冲区
void GetThermoData(float *pBuff);

void GetThermoParams(paramsMLX90640* pBuf);

// mlx90640线程
void mlx90640_task(void* arg);

uint8_t setMLX90640IsPause(uint8_t isPause);

#endif /* _MLX90640_TASK_H_ */
