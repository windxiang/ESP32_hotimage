#include "palette.h"
#include "dispcolor.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

/**
 * @brief 构建多色 伪彩色
 *
 * @param steps 需要生成的个数
 * @param pBuff 输出缓存
 * @param pColor 构建的颜色
 * @param colorSize 颜色个数
 * @return uint16_t 成功生成个数
 */
static uint16_t buildManyColorPalette(uint16_t steps, tRGBcolor* pBuff, tRGBcolor* pColor, uint16_t colorSize)
{
    uint16_t retCount = 0;

    if (!pBuff || !pColor || 0 >= colorSize || 0 >= steps) {
        return retCount;
    }

    uint16_t old_steps = steps; // 原始的个数

    colorSize = colorSize - 1; // 颜色减1 后面使用过程中有取下一个

    // 向上取整
    if (steps % colorSize) {
        steps = (steps / colorSize) * colorSize + colorSize; // 160 = 159 / 5 * 5 + 5
    }

    uint16_t partSize = steps / colorSize; // 总输出个数 / 颜色总个数 = 每段颜色个数

    for (uint16_t part = 0; part < colorSize; part++) { // 颜色个数
        for (uint16_t step = 0; step < partSize; step++) { // 每段个数
            float n = (float)step / (float)(partSize - 1);

            // 求得过度的颜色 = 本段颜色百分比 + 下段颜色百分比
            pBuff->r = (uint8_t)(((float)pColor[part].r) * (1.0f - n) + ((float)pColor[part + 1].r) * n);
            pBuff->g = (uint8_t)(((float)pColor[part].g) * (1.0f - n) + ((float)pColor[part + 1].g) * n);
            pBuff->b = (uint8_t)(((float)pColor[part].b) * (1.0f - n) + ((float)pColor[part + 1].b) * n);

            pBuff++;
            retCount++;
            if (retCount >= old_steps) {
                break;
            }
        }
    }

    return retCount;
}

/**
 * @brief 构建双色 伪彩色
 *
 * @param steps 需要生成的个数
 * @param pBuff 输出缓存
 * @param pColor 构建的颜色
 * @return uint16_t 成功生成个数
 */
static uint16_t buildTwoColorPalette(uint16_t steps, tRGBcolor* pBuff, tRGBcolor* pColor)
{
    uint16_t retCount = 0;

    if (!pBuff || !pColor || 0 >= steps) {
        return retCount;
    }

    for (uint16_t step = 0; step < steps; step++) {
        float n = (float)step / (float)(steps - 1);

        pBuff->r = (uint8_t)(((float)pColor[0].r) * (1.0f - n) + ((float)pColor[1].r) * n);
        pBuff->g = (uint8_t)(((float)pColor[0].g) * (1.0f - n) + ((float)pColor[1].g) * n);
        pBuff->b = (uint8_t)(((float)pColor[0].b) * (1.0f - n) + ((float)pColor[1].b) * n);

        pBuff++;
        retCount++;
    }

    return retCount;
}

/**
 * @brief 返回 所选类型的调色板数组的指针
 *
 * @param palette 调色板类型
 * @param steps 步进
 * @param pBuff 调色板输出指针
 */
void getPalette(eColorScale palette, uint16_t steps, tRGBcolor* pBuff)
{
    switch (palette) {
    case Iron: {
        tRGBcolor KeyColors[] = {
            { 0x00, 0x00, 0x00 }, // 黑色
            { 0x20, 0x00, 0x8C }, // 深蓝色
            { 0xCC, 0x00, 0x77 }, // 紫罗兰
            { 0xFF, 0xD7, 0x00 }, // 金色
            { 0xFF, 0xFF, 0xFF }, // 白色
        };
        buildManyColorPalette(steps, pBuff, KeyColors, sizeof(KeyColors) / sizeof(tRGBcolor));
    } break;

    case Rainbow: {
        tRGBcolor KeyColors[] = {
            { 84, 0, 180 }, // 紫罗兰
            { 0, 97, 211 }, // 蓝色
            { 0, 145, 72 }, // 绿色
            { 207, 214, 0 }, // 黄色
            { 231, 108, 0 }, // 橙色
            { 193, 19, 33 }, // 红色
        };
        buildManyColorPalette(steps, pBuff, KeyColors, sizeof(KeyColors) / sizeof(tRGBcolor));
    } break;

    case Rainbow2: {
        tRGBcolor KeyColors[] = {
            { 143, 0, 255 }, // 紫罗兰
            { 75, 0, 130 }, // 紫罗兰
            { 0, 0, 255 }, // 蓝色
            { 0, 255, 0 }, // 绿色
            { 255, 255, 0 }, // 黄色
            { 255, 127, 0 }, // 橙色
            { 255, 0, 0 }, // 红色
        };
        buildManyColorPalette(steps, pBuff, KeyColors, sizeof(KeyColors) / sizeof(tRGBcolor));
    } break;

    case BlueRed: {
        // 蓝红
        tRGBcolor KeyColors[] = {
            { 0x00, 0x00, 0xFF }, // 蓝色
            { 0xFF, 0x00, 0x00 }, // 红色
        };
        buildTwoColorPalette(steps, pBuff, KeyColors);
    } break;

    case BlackNWhite: {
        // 黑白
        tRGBcolor KeyColors[] = {
            { 0x00, 0x00, 0x00 }, // 黑色
            { 0xFF, 0xFF, 0xFF }, // 白色
        };
        buildTwoColorPalette(steps, pBuff, KeyColors);
    } break;

    case COLOR_MAX:
    default:
        break;
    }
}
