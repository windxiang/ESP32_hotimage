#include "SAFiter.h"
#include <esp_heap_caps.h>

#ifdef CONFIG_ESP32_IIC_SHT31

/**
 * @brief 滑动平均滤波器——创建
 *
 * @param len 滤波器的窗口宽度
 * @return SAFilterHandle_t 返回创建的滤波器句柄
 */
SAFilterHandle_t* SlipAveFilterCreate(uint16_t len)
{
    if (len < 1) {
        len = 1;
    }

    //创建一个滤波器并初始化
    SAFilterHandle_t* newFilter = heap_caps_malloc(sizeof(SAFilterHandle_t), MALLOC_CAP_8BIT);
    newFilter->data = heap_caps_malloc(len * sizeof(float), MALLOC_CAP_8BIT);
    newFilter->len = len;
    newFilter->index = 0;
    newFilter->has = 0;
    newFilter->isfull = 0;
    newFilter->sum = 0;
    newFilter->res = 0;

    return newFilter;
}

/**
 * @brief 滑动平均滤波器——获取结果
 *
 * @param pFilter 滤波器句柄
 * @param input 未滤波的原始数据
 * @return 返回滤波结果
 */
float AddSAFiterRes(SAFilterHandle_t* pFilter, float input)
{
    if (!pFilter) {
        return 0.0f;
    }
    if (!pFilter->isfull) {
        // 还没有存满
        pFilter->has++; // 求当前数组中已有数据数量
        if (pFilter->has == pFilter->len) {
            pFilter->isfull = 1; // 标记数组已满
        }
    } else {
        // 已存满，覆盖写入
        pFilter->sum -= pFilter->data[pFilter->index]; // 先移除最早的数据
    }

    // 写入新的数据
    pFilter->data[pFilter->index] = input;

    // 求当前数组中已有数据的总和
    pFilter->sum += input;

    // 更新下次数据的索引号
    pFilter->index = (pFilter->index == pFilter->len - 1) ? 0 : pFilter->index + 1;

    // 求当前数组中已有数据的平均值
    pFilter->res = pFilter->sum / pFilter->has;

    return pFilter->res;
}

/**
 * @brief 获取滑动平均滤波器结果
 *
 * @param pFilter
 * @return float
 */
float GetSAFiterRes(SAFilterHandle_t* pFilter)
{
    if (!pFilter) {
        return 0.0f;
    }
    return pFilter->res;
}

#endif // CONFIG_ESP32_IIC_SHT31
