#include "CelsiusSymbol.h"
#include "thermalimaging.h"

#define IMAGE_SCALESIZE (10) // LCD缩放倍数
#define TEMP_SCALE (10) // 温度放大倍数
#define RIGHTPALETTEHEIGHT (160) // 右边的比例尺

static int16_t* TermoImage16 = NULL; // 热成像的原始分辨率
static int16_t* TermoHqImage16 = NULL;
static float* gaussBuff = NULL;
static uint16_t PaletteSteps = 0;
static tRGBcolor* pPaletteImage = NULL; // 伪彩色指针
static tRGBcolor* pPaletteScale = NULL; // 右边的伪彩色

// 渲染左下角提示信息
typedef struct _RenderInfoStr {
    char strRenderInfo[256];
    TickType_t tick;
} RenderInfoStr;

static RenderInfoStr renderInfoStr = { 0 };

/**
 * @brief 在窗口左下角显示一行提示
 *
 * @param msgType
 * @param args
 * @param ...
 */
void tips_printf(const char* args, ...)
{
    va_list ap;
    va_start(ap, args);
    vsnprintf(renderInfoStr.strRenderInfo, sizeof(renderInfoStr.strRenderInfo), args, ap);
    va_end(ap);
}

/**
 * @brief 重新生成伪彩色
 *
 */
static void RedrawPalette(float minTemp, float maxTemp)
{
    // 生成一个新的伪彩色
    float delta = (maxTemp - minTemp) * IMAGE_SCALESIZE;
    PaletteSteps = (uint16_t)delta;
    getPalette(settingsParms.ColorScale, PaletteSteps, pPaletteImage);
}

/**
 * @brief 重新生成比例尺
 *
 */
void buildPalette(void)
{
    getPalette(settingsParms.ColorScale, RIGHTPALETTEHEIGHT, pPaletteScale);

    if (!settingsParms.AutoScaleMode) {
        RedrawPalette(settingsParms.minTempNew, settingsParms.maxTempNew);
    }
}

/**
 * @brief 热成像绘图 根据分辨率绘制 (原始分辨率)
 *
 * @param pImage 热成像图 放大10倍
 * @param pPalette 伪彩色
 * @param PaletteSize (最大温度-最小温度)*10+1
 * @param X 绘制起始坐标偏移
 * @param Y 绘制起始坐标偏移
 * @param scaleWidth 放大倍数
 * @param scaleHeight 放大倍数
 * @param minTemp 最小温度
 */
static void DrawImage(int16_t* pImage, tRGBcolor* pPalette, uint16_t PaletteSize, uint16_t X, uint16_t Y, uint8_t scaleWidth, uint8_t scaleHeight, float minTemp)
{
    int cnt = 0;

    for (int row = 0; row < THERMALIMAGE_RESOLUTION_HEIGHT; row++) { // 24行
        for (int col = 0; col < THERMALIMAGE_RESOLUTION_WIDTH; col++, cnt++) { // 32列
            int16_t colorIdx = pImage[cnt] - (minTemp * TEMP_SCALE); // 颜色索引

            if (colorIdx < 0) {
                colorIdx = 0;
            } else if (colorIdx >= PaletteSize) {
                colorIdx = PaletteSize - 1;
            }

            uint16_t color = RGB565(pPalette[colorIdx].r, pPalette[colorIdx].g, pPalette[colorIdx].b);
            dispcolor_FillRect((THERMALIMAGE_RESOLUTION_WIDTH - 1 - col) * scaleWidth + X, row * scaleHeight + Y, scaleHeight, scaleHeight, color);
        }
    }
}

/**
 * @brief 热成像绘图 根据pImage直接显示 (提前做好插值)
 *
 * @param pImage 热成像图
 * @param pPalette 伪彩色
 * @param PaletteSize (最大温度-最小温度)*10+1
 * @param X LCD起始坐标
 * @param Y LCD起始坐标
 * @param width LCD显示宽度
 * @param height LCD显示高度
 * @param minTemp  最小温度
 */
static void DrawHQImage(int16_t* pImage, tRGBcolor* pPalette, uint16_t PaletteSize, uint16_t X, uint16_t Y, uint16_t width, uint16_t height, float minTemp)
{
    int cnt = 0;

    for (int row = 0; row < height; row++) {
        for (int col = 0; col < width; col++) {
            int16_t colorIdx = pImage[cnt] - (int16_t)(minTemp * TEMP_SCALE);
            cnt++;

            if (colorIdx < 0) {
                colorIdx = 0;
            } else if (colorIdx >= PaletteSize) {
                colorIdx = PaletteSize - 1;
            }

            uint16_t color = RGB565(pPalette[colorIdx].r, pPalette[colorIdx].g, pPalette[colorIdx].b);
            dispcolor_DrawPixel((width - col - 1) + X, row + Y, color);
        }
    }
}

/**
 * @brief 绘制右边的伪彩色 色条
 *
 * @param X 起始坐标
 * @param Y 起始坐标
 * @param Width 宽度
 * @param Height 高度
 * @param minTemp 最小温度
 * @param maxTemp 最高温度
 */
static void DrawPalette(uint16_t X, uint16_t Y, uint16_t Width, uint16_t Height, float minTemp, float maxTemp)
{
#define PRINTFCHAR "%.1f%s"
    // 绘制颜色尺
    for (int i = 0; i < Height; i++) {
        uint16_t color = RGB565(pPaletteScale[i].r, pPaletteScale[i].g, pPaletteScale[i].b);
        dispcolor_FillRect(X, Y + Height - i - 1, Width, 1, color);
    }

    // 绘制边框
    dispcolor_DrawRectangle(X, Y, X + Width, Y + Height, BLACK);

    // 显示刻度中的最大值（水平-居中）
    int16_t TextWidth = dispcolor_getFormatStrWidth(FONTID_6X8M, PRINTFCHAR, maxTemp, CELSIUS_SYMBOL);
    dispcolor_printf(X + (Width - TextWidth) / 2, Y - 8, FONTID_6X8M, settingsParms.ColorScale == Rainbow || settingsParms.ColorScale == Rainbow2 || settingsParms.ColorScale == BlueRed ? WHITE : RED, PRINTFCHAR, maxTemp, CELSIUS_SYMBOL);

    // 显示刻度中的最小值（水平-居中）
    TextWidth = dispcolor_getFormatStrWidth(FONTID_6X8M, PRINTFCHAR, minTemp, CELSIUS_SYMBOL);
    dispcolor_printf(X + (Width - TextWidth) / 2, Y + Height + 3, FONTID_6X8M, WHITE, PRINTFCHAR, minTemp, CELSIUS_SYMBOL);
#undef PRINTFCHAR
}

/**
 * @brief 绘制中心十字准线
 *
 * @param cX
 * @param cY
 * @param Temp
 * @param color
 */
static void DrawCenterTempColor(uint16_t cX, uint16_t cY, float Temp, uint16_t color)
{
    uint8_t offMin = 5; // 起点
    uint8_t offMax = 10; // 终点
    uint8_t offTwin = 1; // 2根线之间的距离

    // Top of the crosshair
    dispcolor_DrawLine(cX - offTwin, cY - offMin, cX - offTwin, cY - offMax, color);
    dispcolor_DrawLine(cX + offTwin, cY - offMin, cX + offTwin, cY - offMax, color);
    // Bottom of the crosshair
    dispcolor_DrawLine(cX - offTwin, cY + offMin, cX - offTwin, cY + offMax, color);
    dispcolor_DrawLine(cX + offTwin, cY + offMin, cX + offTwin, cY + offMax, color);
    // Left side of the crosshair
    dispcolor_DrawLine(cX - offMin, cY - offTwin, cX - offMax, cY - offTwin, color);
    dispcolor_DrawLine(cX - offMin, cY + offTwin, cX - offMax, cY + offTwin, color);
    // Right side of the crosshair
    dispcolor_DrawLine(cX + offMin, cY - offTwin, cX + offMax, cY - offTwin, color);
    dispcolor_DrawLine(cX + offMin, cY + offTwin, cX + offMax, cY + offTwin, color);

    // 显示文本
    if ((Temp > -100) && (Temp < 500)) {
        dispcolor_printf(cX + 8, cY + 8, FONTID_6X8M, color, "%.1f%s", Temp, CELSIUS_SYMBOL);
    } else {
        dispcolor_printf(cX + 8, cY + 8, FONTID_6X8M, color, "Error Temp");
    }
}

/**
 * @brief 在屏幕中心 绘制中心点温度 和 中心十字准线
 *
 * @param X 偏移坐标X
 * @param Y 偏移坐标Y
 * @param Width 热成像图 总宽
 * @param Height 热成像图 总高
 * @param CenterTemp 中心点温度
 */
static void DrawCenterTemp(uint16_t X, uint16_t Y, uint16_t Width, uint16_t Height, float CenterTemp)
{
    // 屏幕中间 + 偏移坐标
    uint16_t cX = (Width >> 1) + X;
    uint16_t cY = (Height >> 1) + Y;

    // 渲染阴影黑色
    DrawCenterTempColor(cX + 1, cY + 1, CenterTemp, BLACK);
    // 渲染白色
    DrawCenterTempColor(cX, cY, CenterTemp, WHITE);
}

/**
 * @brief 根据不同插值算法 计算标记点的坐标偏移
 *
 * @param pX
 * @param pY
 */
static void ThermoToImagePosition(int16_t* pX, int16_t* pY)
{
    switch (settingsParms.ScaleMode) {
    case ORIGINAL:
        *pX = (*pX) * IMAGE_SCALESIZE;
        *pY = (*pY) * IMAGE_SCALESIZE;
        break;

    case LINEAR:
        *pX = (*pX) * IMAGE_SCALESIZE;
        *pY = (*pY) * IMAGE_SCALESIZE;
        break;

    case HQ3X_2X:
        *pX = (*pX) * IMAGE_SCALESIZE;
        *pY = (*pY) * IMAGE_SCALESIZE;
        break;
    }
}

/**
 * @brief 在热成像上 标记最大 最小 温度值的点
 *
 * @param pMlxData MLX90640数据
 */
static void DrawMarkers(sMlxData* pMlxData)
{
    uint8_t lineHalf = 4;

    // 绘制* 最大温度
    int16_t x = THERMALIMAGE_RESOLUTION_WIDTH - pMlxData->maxT_X - 1; // 热成像从右到左 要取反
    int16_t y = pMlxData->maxT_Y;
    ThermoToImagePosition(&x, &y);

    uint16_t mainColor = RED;
    dispcolor_DrawLine(x + 1, y - lineHalf + 1, x + 1, y + lineHalf + 1, BLACK); // 阴影黑色
    dispcolor_DrawLine(x - lineHalf + 1, y + 1, x + lineHalf + 1, y + 1, BLACK);
    dispcolor_DrawLine(x - lineHalf + 1, y - lineHalf + 1, x + lineHalf + 1, y + lineHalf + 1, BLACK);
    dispcolor_DrawLine(x - lineHalf + 1, y + lineHalf + 1, x + lineHalf + 1, y - lineHalf + 1, BLACK);

    dispcolor_DrawLine(x, y - lineHalf, x, y + lineHalf, mainColor);
    dispcolor_DrawLine(x - lineHalf, y, x + lineHalf, y, mainColor);
    dispcolor_DrawLine(x - lineHalf, y - lineHalf, x + lineHalf, y + lineHalf, mainColor);
    dispcolor_DrawLine(x - lineHalf, y + lineHalf, x + lineHalf, y - lineHalf, mainColor);

    // 绘制X 最小温度
    x = THERMALIMAGE_RESOLUTION_WIDTH - pMlxData->minT_X - 1;
    y = pMlxData->minT_Y;
    ThermoToImagePosition(&x, &y);

    mainColor = RGB565(0, 200, 245);
    dispcolor_DrawLine(x - lineHalf + 1, y - lineHalf + 1, x + lineHalf + 1, y + lineHalf + 1, BLACK); // 阴影黑色
    dispcolor_DrawLine(x - lineHalf + 1, y + lineHalf + 1, x + lineHalf + 1, y - lineHalf + 1, BLACK);
    dispcolor_DrawLine(x - lineHalf, y - lineHalf, x + lineHalf, y + lineHalf, mainColor); // 蓝色
    dispcolor_DrawLine(x - lineHalf, y + lineHalf, x + lineHalf, y - lineHalf, mainColor);
}

/**
 * @brief 在热成像上 标记最大 最小 温度值的点
 * 高斯模糊 双线性插值
 * @param pMlxData
 */
static void DrawMarkersHQ(sMlxData* pMlxData)
{
    uint8_t lineHalf = 4;

    int16_t x = THERMALIMAGE_RESOLUTION_WIDTH - pMlxData->maxT_X - 1;
    int16_t y = pMlxData->maxT_Y;
    ThermoToImagePosition(&x, &y);

    // 绘制* 最大温度
    uint16_t mainColor = RED;
    dispcolor_DrawLine(x + 1, y - lineHalf + 1, x + 1, y + lineHalf + 1, BLACK);
    dispcolor_DrawLine(x - lineHalf + 1, y + 1, x + lineHalf + 1, y + 1, BLACK);
    dispcolor_DrawLine(x - lineHalf + 1, y - lineHalf + 1, x + lineHalf + 1, y + lineHalf + 1, BLACK);
    dispcolor_DrawLine(x - lineHalf + 1, y + lineHalf + 1, x + lineHalf + 1, y - lineHalf + 1, BLACK);

    dispcolor_DrawLine(x, y - lineHalf, x, y + lineHalf, mainColor);
    dispcolor_DrawLine(x - lineHalf, y, x + lineHalf, y, mainColor);
    dispcolor_DrawLine(x - lineHalf, y - lineHalf, x + lineHalf, y + lineHalf, mainColor);
    dispcolor_DrawLine(x - lineHalf, y + lineHalf, x + lineHalf, y - lineHalf, mainColor);

    // 绘制X 最小温度
    x = THERMALIMAGE_RESOLUTION_WIDTH - pMlxData->minT_X - 1;
    y = pMlxData->minT_Y;
    ThermoToImagePosition(&x, &y);

    mainColor = RGB565(0, 200, 245);
    dispcolor_DrawLine(x - lineHalf + 1, y - lineHalf + 1, x + lineHalf + 1, y + lineHalf + 1, BLACK);
    dispcolor_DrawLine(x - lineHalf + 1, y + lineHalf + 1, x + lineHalf + 1, y - lineHalf + 1, BLACK);
    dispcolor_DrawLine(x - lineHalf, y - lineHalf, x + lineHalf, y + lineHalf, mainColor);
    dispcolor_DrawLine(x - lineHalf, y + lineHalf, x + lineHalf, y - lineHalf, mainColor);
}

/**
 * @brief 绘制电池图标
 *
 * @param X
 * @param Y
 * @param VBAT 电池电压 V
 */
static void DrawBattery(uint16_t X, uint16_t Y)
{
    float VBAT = ((float)getBatteryVoltage()) / 1000;
    int8_t isCharge = getBatteryCharge();

    // printf("%f\r\n", VBAT);
    // dispcolor_printf_Bg(210, 4, FONTID_6X8M, RGB565(160, 96, 0), BLACK, "Vbat=%.2fV", VBAT);

#if 0
    float capacity = VBAT * 125 - 400;
    uint16_t Color = GREEN;

    if (capacity > 100) {
        capacity = 100;
    } else if (capacity < 0) {
        capacity = 0;
    }

    if (capacity < 50) {
        Color = RED;
    } else if (capacity < 80) {
        Color = RGB565(249, 166, 2);
    }
#endif
    // 画出电池的轮廓
    dispcolor_DrawRectangle(X, Y, X + 22, Y + 10, WHITE);
    dispcolor_DrawRectangleFilled(X + 22, Y + 3, X + 25, Y + 7, WHITE); // 电池头

#if 0
    // 绘制电池小格子
    dispcolor_DrawRectangleFilled(X + 17, Y + 2, X + 18, Y + 7, capacity < 85.8f ? GRAY : Color); // 第六格
    dispcolor_DrawRectangleFilled(X + 14, Y + 2, X + 15, Y + 7, capacity < 71.5f ? GRAY : Color);
    dispcolor_DrawRectangleFilled(X + 11, Y + 2, X + 12, Y + 7, capacity < 57.2f ? GRAY : Color); //

    dispcolor_DrawRectangleFilled(X + 8, Y + 2, X + 9, Y + 7, capacity < 42.9f ? GRAY : Color); //
    dispcolor_DrawRectangleFilled(X + 5, Y + 2, X + 6, Y + 7, capacity < 28.6f ? GRAY : Color); //
    dispcolor_DrawRectangleFilled(X + 2, Y + 2, X + 3, Y + 7, capacity < 14.3f ? GRAY : Color); // 第一格
#endif

    // 绘制电池百分比
    const float offsetMin = 3.7f, offsetMax = 4.2f;
    float VbatPer = (VBAT - offsetMin) / (offsetMax - offsetMin) * 100; // 电池当前百分比

    if (VbatPer > 100) {
        VbatPer = 100.0f;
    } else if (VbatPer < 0) {
        VbatPer = 0.0f;
    }

    if (VbatPer == 100.0f) {
        dispcolor_printf(X + 3, Y + 2, FONTID_6X8M, WHITE, "%.0f", VbatPer);
    } else if (10 <= VbatPer && VbatPer <= 90) {
        dispcolor_printf(X + 6, Y + 2, FONTID_6X8M, WHITE, "%.0f", VbatPer);
    } else {
        dispcolor_printf(X + 10, Y + 2, FONTID_6X8M, WHITE, "%.0f", VbatPer);
    }

    // 是否充电中
    if (0 == isCharge) {
        dispcolor_DrawCircleFilled(X - 10, Y + 5, 3, RED);
    }
}

/**
 * @brief 用于缩放算法、源图像和最终图像的缓冲区
 *
 * @return int8_t
 */
static int8_t AllocThermoImageBuffers(void)
{
    TermoImage16 = heap_caps_malloc((THERMALIMAGE_RESOLUTION_WIDTH * THERMALIMAGE_RESOLUTION_HEIGHT) << 1, MALLOC_CAP_8BIT); // 热成像的原始分辨率
    TermoHqImage16 = heap_caps_malloc((dispcolor_getWidth() * dispcolor_getHeight()) << 1, MALLOC_CAP_8BIT); // 线性缩放
    gaussBuff = heap_caps_malloc(((THERMALIMAGE_RESOLUTION_WIDTH * 2) * (THERMALIMAGE_RESOLUTION_HEIGHT * 2)) * sizeof(float), MALLOC_CAP_8BIT /*| MALLOC_CAP_SPIRAM*/); // 高斯缩放

    pPaletteImage = heap_caps_malloc(((MAX_TEMP - MIN_TEMP) * IMAGE_SCALESIZE) << 1, MALLOC_CAP_8BIT); // 伪彩色
    pPaletteScale = heap_caps_malloc((THERMALIMAGE_RESOLUTION_HEIGHT * IMAGE_SCALESIZE) << 1, MALLOC_CAP_8BIT); // 右边显示的伪彩色条

    if (!TermoImage16 || !TermoHqImage16 || !gaussBuff || !pPaletteImage || !pPaletteScale)
        return -1;
    return 0;
}

/**
 * @brief 绘制标题
 *
 */
static void drawTitleItem(sMlxData* pMlxData)
{
    int16_t offsetX = 0;

    // 在左上角绘制最大温度 和 最小温度
    if (settingsParms.TempMarkers) {
        if ((pMlxData->maxT > -100) && (pMlxData->maxT < 500)) {
            offsetX = dispcolor_printf(0, 4, FONTID_6X8M, WHITE, "Max:%.1f%s", pMlxData->maxT, CELSIUS_SYMBOL);
        } else {
            offsetX = dispcolor_printf(0, 4, FONTID_6X8M, WHITE, "Max:Error");
        }
        offsetX += 10;

        if ((pMlxData->minT > -100) && (pMlxData->minT < 500)) {
            dispcolor_printf(0, 14, FONTID_6X8M, WHITE, "Min:%.1f%s", pMlxData->minT, CELSIUS_SYMBOL);
        } else {
            dispcolor_printf(0, 14, FONTID_6X8M, WHITE, "Min:Error");
        }
    }

    // 绘制刷新率
    // dispcolor_printf(offsetX, 4, FONTID_6X8M, WHITE, "%.1f FPS\r\n", FPS_RATES[settingsParms.MLX90640FPS]); // 这个是热成像的帧率

    // 计算并显示电池的电量和电压
    DrawBattery(290, 3);
}

/**
 * @brief 绘制底部内容
 *
 */
static void drawBottomItem(sMlxData* pMlxData)
{
    int16_t offsetY = 232;
    int16_t offsetX = dispcolor_printf_Bg(0, offsetY, FONTID_6X8M, RGB565(0, 160, 160), BLACK, "Ta:%.1f%s ", pMlxData->Ta, CELSIUS_SYMBOL);
    // 当前辐射率
    offsetX = dispcolor_printf_Bg(offsetX, offsetY, FONTID_6X8M, RGB565(96, 160, 0), BLACK, "E:%.2f ", settingsParms.Emissivity);
    // 电压
    offsetX = dispcolor_printf_Bg(offsetX, offsetY, FONTID_6X8M, RGB565(0, 160, 160), BLACK, "Vdd:%.1fV ", pMlxData->Vdd);
#ifdef CONFIG_ESP32_IIC_SHT31
    // 绘制SHT31内容
    offsetX = dispcolor_printf_Bg(offsetX, offsetY, FONTID_6X8M, RGB565(0, 160, 160), BLACK, "SHT31:%.1f%s/%.1fRH ", sht31_getTemperature(), CELSIUS_SYMBOL, sht31_getHumidity());
#endif
}

/**
 * @brief 计算最大温度 最小温度 中间温度
 *
 */
static void CalcTempFromMLX90640(sMlxData* pMlxData)
{
    const float* pThermoImage = pMlxData->ThermoImage;

    // 计算屏幕中心的温度 累加中间4个像素的值
    pMlxData->CenterTemp = //
        pThermoImage[THERMALIMAGE_RESOLUTION_WIDTH * ((THERMALIMAGE_RESOLUTION_HEIGHT >> 1) - 1) + ((THERMALIMAGE_RESOLUTION_WIDTH >> 1) - 1)] + // (32 * (24 / 2 - 1)) + (32 / 2 - 1)
        pThermoImage[THERMALIMAGE_RESOLUTION_WIDTH * ((THERMALIMAGE_RESOLUTION_HEIGHT >> 1) - 1) + (THERMALIMAGE_RESOLUTION_WIDTH >> 1)] + // (32 * (24 / 2 - 1)) + (32 / 2)
        pThermoImage[THERMALIMAGE_RESOLUTION_WIDTH * (THERMALIMAGE_RESOLUTION_HEIGHT >> 1) + ((THERMALIMAGE_RESOLUTION_WIDTH >> 1) - 1)] + // (32 * (24 / 2)) + (32 / 2 - 1)
        pThermoImage[THERMALIMAGE_RESOLUTION_WIDTH * (THERMALIMAGE_RESOLUTION_HEIGHT >> 1) + (THERMALIMAGE_RESOLUTION_WIDTH >> 1)]; // (32 * (24 / 2)) + (32 / 2)
    pMlxData->CenterTemp /= 4;

    // 搜索帧中的最小和最大温度 及坐标
    pMlxData->minT = MAX_TEMP;
    pMlxData->maxT = MIN_TEMP;

    for (uint8_t y = 0; y < THERMALIMAGE_RESOLUTION_HEIGHT; y++) {
        for (uint8_t x = 0; x < THERMALIMAGE_RESOLUTION_WIDTH; x++) {
            float temp = pThermoImage[y * THERMALIMAGE_RESOLUTION_WIDTH + x]; // 得到像素温度

            if (pMlxData->maxT < temp) {
                pMlxData->maxT = temp;
                pMlxData->maxT_X = x;
                pMlxData->maxT_Y = y;
            }

            if (pMlxData->minT > temp) {
                pMlxData->minT = temp;
                pMlxData->minT_X = x;
                pMlxData->minT_Y = y;
            }
        }
    }

    if (pMlxData->maxT > MAX_TEMP) {
        pMlxData->maxT = MAX_TEMP;
    }

    if (pMlxData->minT < MIN_TEMP) {
        pMlxData->minT = MIN_TEMP;
    }
}

/**
 * @brief UI线程
 *
 * @param arg
 */
void render_task(void* arg)
{
    const uint16_t MLX90640PIXSIZE = THERMALIMAGE_RESOLUTION_WIDTH * THERMALIMAGE_RESOLUTION_HEIGHT; // MLX90640总像素大小
    float minTemp = MAX_TEMP, maxTemp = MIN_TEMP;

    // 用于缩放算法、源图像和最终图像的缓冲区
    if (AllocThermoImageBuffers()) {
        goto error;
    }

    // 全屏显示黑色
    dispcolor_ClearScreen();

    // 刷新右边的比例尺
    buildPalette();

    while (1) {
        EventBits_t uxBitsToWaitFor = RENDER_MLX90640_NO0 | RENDER_MLX90640_NO1 | RENDER_ShortPress_Up | RENDER_Hold_Up | RENDER_ShortPress_Center | RENDER_Hold_Center | RENDER_ShortPress_Down | RENDER_Hold_Down;
        EventBits_t bits = xEventGroupWaitBits(pHandleEventGroup, uxBitsToWaitFor, pdFALSE, pdFALSE, portMAX_DELAY);
        xEventGroupClearBits(pHandleEventGroup, bits);

        if ((bits & RENDER_MLX90640_NO0) == RENDER_MLX90640_NO0 || (bits & RENDER_MLX90640_NO1) == RENDER_MLX90640_NO1) {
            int8_t idx = ((int8_t)bits >> 1) & 1;
            sMlxData* _pMlxData = &pMlxData[idx];

            // 计算最大温度 最小温度 中间温度
            CalcTempFromMLX90640(_pMlxData);

            if (settingsParms.AutoScaleMode) {
                // 自动计算每帧的 最大温度 最小温度 的伪彩色
                if (_pMlxData->minT != minTemp || _pMlxData->maxT != maxTemp) {
                    settingsParms.minTempNew = minTemp = _pMlxData->minT;
                    settingsParms.maxTempNew = maxTemp = _pMlxData->maxT;
                    RedrawPalette(minTemp, maxTemp);
                    // printf("%.1f %.1f \r\n", minTemp, maxTemp);
                }

            } else {
                minTemp = _pMlxData->minT;
                maxTemp = _pMlxData->maxT;
            }

            // 将温度复制到一个整数数组，以简化进一步的计算
            for (uint16_t i = 0; i < MLX90640PIXSIZE; i++) {
                TermoImage16[i] = _pMlxData->ThermoImage[i] * TEMP_SCALE;
            }

            // 显示热图
            switch (settingsParms.ScaleMode) {
            case ORIGINAL:
                // 原始 只放大SCALE_SIZE倍
                DrawImage(TermoImage16, pPaletteImage, PaletteSteps, 0, 0, IMAGE_SCALESIZE, IMAGE_SCALESIZE, minTemp);
                break;

            case LINEAR:
                // 线性插值
                idwOldInterpolate(TermoImage16, THERMALIMAGE_RESOLUTION_WIDTH, THERMALIMAGE_RESOLUTION_HEIGHT, IMAGE_SCALESIZE, TermoHqImage16);
                DrawHQImage(TermoHqImage16, pPaletteImage, PaletteSteps, 0, 0, dispcolor_getWidth(), dispcolor_getHeight(), minTemp);
                break;

            case HQ3X_2X:
                // 高斯模糊 双线性插值
                idwGauss(TermoImage16, THERMALIMAGE_RESOLUTION_WIDTH, THERMALIMAGE_RESOLUTION_HEIGHT, 2, gaussBuff);
                idwBilinear(gaussBuff, THERMALIMAGE_RESOLUTION_WIDTH * 2, THERMALIMAGE_RESOLUTION_HEIGHT * 2, TermoHqImage16, dispcolor_getWidth(), dispcolor_getHeight(), IMAGE_SCALESIZE / 2);
                DrawHQImage(TermoHqImage16, pPaletteImage, PaletteSteps, 0, 0, dispcolor_getWidth(), dispcolor_getHeight(), minTemp);
                break;
            }

            // 热图上的最大/最小标记
            if (settingsParms.TempMarkers) {
                // 在屏幕中央显示温度
                DrawCenterTemp(0, 0, dispcolor_getWidth(), dispcolor_getHeight(), _pMlxData->CenterTemp);

                // 标记最大 最小 点
                if (settingsParms.ScaleMode == HQ3X_2X) {
                    DrawMarkersHQ(_pMlxData);
                } else {
                    DrawMarkers(_pMlxData);
                }
            }

            // 绘制标题内容
            drawTitleItem(_pMlxData);

            // 绘制底部内容
            drawBottomItem(_pMlxData);

            // 绘制右边的伪彩色
            DrawPalette(dispcolor_getWidth() - 25, (dispcolor_getHeight() >> 1) - 80, 15, RIGHTPALETTEHEIGHT, settingsParms.minTempNew, settingsParms.maxTempNew);
        }

        if ((bits & RENDER_ShortPress_Up) == RENDER_ShortPress_Up) {
            // Up 短按
            FuncUp_Run();
        }
        if ((bits & RENDER_Hold_Up) == RENDER_Hold_Up) {
            // Up 长按 进入睡眠模式
            Deep_Sleep_Run();
        }

        if ((bits & RENDER_ShortPress_Center) == RENDER_ShortPress_Center) {
            // Center 短按
            FuncCenter_Run();
        }
        if ((bits & RENDER_Hold_Center) == RENDER_Hold_Center) {
            // Center长按 - 菜单渲染
            menu_run();
            settings_write_all();
            dispcolor_ClearScreen();
        }

        if ((bits & RENDER_ShortPress_Down) == RENDER_ShortPress_Down) {
            // Down短按
            FuncDown_Run();
        }
        if ((bits & RENDER_Hold_Down) == RENDER_Hold_Down) {
            // Down长按
        }

        // 显示左下角Tips信息
        if (strlen(renderInfoStr.strRenderInfo) > 0) {
            TickType_t tick = pdTICKS_TO_MS(xTaskGetTickCount());
            if (renderInfoStr.tick == 0) {
                renderInfoStr.tick = tick;
            }

            if (tick - renderInfoStr.tick < 3000) {
                // 显示
                dispcolor_printf_Bg(0, 222, FONTID_6X8M, RGB565(96, 160, 0), BLACK, "%s", renderInfoStr.strRenderInfo);

            } else {
                // 不显示
                memset(renderInfoStr.strRenderInfo, 0, sizeof(renderInfoStr.strRenderInfo));
                renderInfoStr.tick = 0;
            }
        }

        // 把显存内容刷新到液晶屏上
        dispcolor_Update();
    }

error:
    if (NULL != pPaletteImage) {
        heap_caps_free(pPaletteImage);
        pPaletteImage = NULL;
    }
    if (NULL != TermoImage16) {
        heap_caps_free(TermoImage16);
        TermoImage16 = NULL;
    }
    if (NULL != TermoHqImage16) {
        heap_caps_free(TermoHqImage16);
        TermoHqImage16 = NULL;
    }
    if (NULL != gaussBuff) {
        heap_caps_free(gaussBuff);
        gaussBuff = NULL;
    }
    vTaskDelete(NULL);
    FatalErrorMsg("Error mlx tasks\r\n");
}
