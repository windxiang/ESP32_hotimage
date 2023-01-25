#include "IDW.h"
#include "esp_log.h"

// 设置指定像素的颜色值
static inline void set_point(int16_t* pSrc, uint16_t width, uint16_t height, int16_t x, int16_t y, float f)
{
    pSrc[y * width + x] = (int16_t)f;
}

// 获取指定像素的颜色值
static float bilinear_get_point(const float* p, uint8_t width, uint8_t height, int8_t x, int8_t y)
{
    if (x >= width) {
        x = width - 1;
    }

    if (y >= height) {
        y = height - 1;
    }
    return p[y * width + x];
}

// 将4个像素融合在一起
static float bilinear_interpolate(const float p[], float x, float y)
{
    float xx = p[1] * x + p[0] * (1.f - x);
    float xy = p[3] * x + p[2] * (1.f - x);
    return xy * y + xx * (1.f - y);
}

// 得到4个像素值
static void bilinear_get_adjacents_2d(const float* pSrc, float* pDest, uint8_t src_gauss_width, uint8_t src_gauss_height, int8_t x, int8_t y)
{
    pDest[0] = bilinear_get_point(pSrc, src_gauss_width, src_gauss_height, x + 0, y + 0); // 原点
    pDest[1] = bilinear_get_point(pSrc, src_gauss_width, src_gauss_height, x + 1, y + 0); // 右边
    pDest[2] = bilinear_get_point(pSrc, src_gauss_width, src_gauss_height, x + 0, y + 1); // 上边
    pDest[3] = bilinear_get_point(pSrc, src_gauss_width, src_gauss_height, x + 1, y + 1); // 右下
}

/**
 * @brief 双线性插值
 *
 * @param pSrcGaussBuf 高斯模糊地址
 * @param src_gauss_width 高斯模糊宽
 * @param src_gauss_height 高斯模糊高
 * @param pDest 输出图像地址
 * @param dest_width 输出图像宽
 * @param dest_height 输出图像高
 * @param upScaleFactor 比例因子
 */
void idwBilinear(const float* pSrcGaussBuf, uint8_t src_gauss_width, uint8_t src_gauss_height, int16_t* pDest, uint16_t dest_width, uint16_t dest_height, uint8_t upScaleFactor)
{
    const float mu = 1.f / upScaleFactor;
    float adj_2d[4]; // 存储相邻矩阵

    for (uint16_t y_idx = 0; y_idx < dest_height; y_idx++) {
        for (uint16_t x_idx = 0; x_idx < dest_width; x_idx++) {
            float x = x_idx * mu;
            float y = y_idx * mu;
            bilinear_get_adjacents_2d(pSrcGaussBuf, adj_2d, src_gauss_width, src_gauss_height, (int8_t)x, (int8_t)y);

            float frac_x = x - (int)x; // 我们只需要点之间的~delta~
            float frac_y = y - (int)y; // 我们只需要点之间的~delta~
            float out = bilinear_interpolate(adj_2d, frac_x, frac_y);

            set_point(pDest, dest_width, dest_height, x_idx, y_idx, out);
        }
    }
}

#if 0
/**
 * @brief 打印输出一行数据
 *
 * @param pSrcImage
 * @param pHDImage
 * @param srcCount
 * @param hdCount
 */
void dumpLineData(int16_t lineNo, int16_t* pSrcImage, int16_t* pHDImage, int16_t srcCount, int16_t hdCount)
{
    printf("------------------------------ src  %d\r\n", lineNo);
    for (int16_t i = 0; i < srcCount; i++) {
        printf("%04X,", pSrcImage[i]);
    }
    printf("\r\n");

    printf("hd \r\n");
    for (int16_t i = 0; i < hdCount; i++) {
        printf("%04X,", pHDImage[i]);
        if (i != 0 && (i % 10) == 0) {
            printf("\r\n");
        }
    }
    printf("\r\n");
}
#endif

/**
 * @brief 老双线性插值
 *
 * @param src 输入图像地址
 * @param src_rows 宽
 * @param src_cols 高
 * @param steps 缩放倍数
 * @param pHdImage 输出图像地址
 */
void idwOldInterpolate(int16_t* pSrcImage, uint16_t srcWidth, uint16_t srcHeight, uint16_t scale, int16_t* pHDImageOut)
{

    const uint16_t scaleWidth = srcWidth * scale; // 一行LCD像素个数 = 原始宽度 * 放大比例

    /*
                      0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 ...... 317 318 319    共320列
    第 00 行 插值     0~9=pSrcImage[0][0-1插值]，  10~19=pSrcImage[0][1-2插值]，  20~29=pSrcImage[0][2-3插值]， ....... 31=pSrcImage[0][319]
    第 yy 行 不处理
    第 10 行 插值     0~9=pSrcImage[1][0-1插值]，  10~19=pSrcImage[1][1-2插值]，  20~29=pSrcImage[1][2-3插值]， .......
    第 yy 行 不处理
    第 20 行 插值     0~9=pSrcImage[2][0-1插值]，  10~19=pSrcImage[2][1-2插值]，  20~29=pSrcImage[2][2-3插值]， .......
    第 yy 行 不处理
    第 30 行 插值     0~9=pSrcImage[3][0-1插值]，  10~19=pSrcImage[3][1-2插值]，  20~29=pSrcImage[3][2-3插值]， .......
    第 yy 行 不处理
    第 x0 行 插值
    第 yy 行 不处理
    第230 行 插值     0~9=pSrcImage[22][0-1插值]， 10~19=pSrcImage[22][1-2插值]， 20~29=pSrcImage[22][2-3插值]， .......
    第 yy 行 不处理
    第239 行 插值     0~9=pSrcImage[23][0-1插值]， 10~19=pSrcImage[23][1-2插值]， 20~29=pSrcImage[23][2-3插值]， .......
    共240行
     */
    // 计算分布到每行的次数
    const uint16_t distributedX = scaleWidth - ((srcWidth - 1) * scale) - 1;
    const uint16_t distributedY = (srcHeight * scale) - ((srcHeight - 1) * scale) - 1;

    // 先插值每行的数据
    int32_t offsetY = 0;
    uint16_t _newSrcWidth = srcWidth - 1;
    for (uint16_t _h = 0; _h < srcHeight; _h++) { // 高度24行
        // 得到原始像素偏移
        uint16_t baseS = _h * srcWidth; // 第几行 * 一行像素个数 = 原始温度位置

        // 计算目标地址      例: 0~319  3200~3519  6400~6719  9600~9919
        // 每隔scale行计算一次 0-10-20-30  一共计算srcHeight次
        uint32_t offsetX = 0;
        if (_h == (srcHeight - 1)) {
            // 最后一行
            offsetX = (srcHeight * scale) * scaleWidth - scaleWidth;
        } else {
            offsetX = (_h * scale * scaleWidth) + offsetY;
        }

        // 插值一行数据  32 -> 320
        for (uint16_t _w = 0; _w < _newSrcWidth; _w++) { // 宽度 一行有32列
            // 得到左右相邻的2个温度
            int16_t tempStart = pSrcImage[baseS + _w];
            int16_t tempEnd = pSrcImage[baseS + _w + 1];

            int16_t _newScale = scale;
            if (_w <= distributedX) {
                _newScale++;
            }

            // 开始插值计算
            for (uint16_t _s = 0; _s < _newScale; _s++) {
                pHDImageOut[offsetX + _s] = (tempStart * (_newScale - _s) + tempEnd * _s) / _newScale;
            }
            offsetX += _newScale;
        }

        // 计算前几行要分布的偏移
        if (_h < distributedY) {
            offsetY += scaleWidth;
        }
    }

    /*
    将上面最后9行分布到前面几行中，每个各分配1行，凑满9行
    0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 ...... 317 318 319    共320列
    第 00 行 插值     0~9=pSrcImage[0][0-1插值]，  10~19=pSrcImage[0][1-2插值]，  20~29=pSrcImage[0][2-3插值]， .......
    第 yy 行   现在处理 一列一列进行处理
    第 10 行 插值     0~9=pSrcImage[1][0-1插值]，  10~19=pSrcImage[1][1-2插值]，  20~29=pSrcImage[1][2-3插值]， .......
    第 yy 行   现在处理 一列一列进行处理
    第 20 行 插值     0~9=pSrcImage[2][0-1插值]，  10~19=pSrcImage[2][1-2插值]，  20~29=pSrcImage[2][2-3插值]， .......
    第 yy 行   现在处理 一列一列进行处理
    第 30 行 插值     0~9=pSrcImage[3][0-1插值]，  10~19=pSrcImage[3][1-2插值]，  20~29=pSrcImage[3][2-3插值]， .......
    第 yy 行   现在处理 一列一列进行处理
    第 x0 行 插值
    第 yy 行   现在处理 一列一列进行处理
    第230 行 插值     0~9=pSrcImage[22][0-1插值]， 10~19=pSrcImage[22][1-2插值]， 20~29=pSrcImage[22][2-3插值]， .......
    第 yy 行   现在处理 一列一列进行处理
    第239 行 插值     0~9=pSrcImage[23][0-1插值]， 10~19=pSrcImage[23][1-2插值]， 20~29=pSrcImage[23][2-3插值]， .......
    共240行
     */
    // 插值2行之间的数据
    uint16_t _newScale; // 可以认为是写入的行数
    uint16_t _newSrcHeight = srcHeight - 1;
    int32_t _s = 0, _n = 0;

    for (uint16_t _w = 0; _w < scaleWidth; _w++) { // 320列
        _s = _n = _w;

        for (uint16_t _h = 0; _h < _newSrcHeight; _h++) { // 24行
            // 计算2行的偏移位置
            _s = _n;

            if (_h < distributedY) {
                _n += scaleWidth * scale + scaleWidth;
                _newScale = scale + 1;
            } else {
                _n += scaleWidth * scale;
                _newScale = scale;
            }

            // 得到2行之间的温度
            int16_t tempStart = pHDImageOut[_s]; // 行 * 10 * 320 + 列
            int16_t tempEnd = pHDImageOut[_n]; // 下一行

            // outputDebug("%d %08X %08X %x %x\r\n", _h, pHDImageOut + _s, pHDImageOut + _n, tempStart, tempEnd);
            for (uint16_t _scale = 1; _scale < _newScale; _scale++) {
                uint32_t Idx = _s + _scale * scaleWidth;
                pHDImageOut[Idx] = tempStart * (_newScale - _scale) / _newScale + tempEnd * _scale / _newScale; // 插值算法
            }
        }
    }
}
