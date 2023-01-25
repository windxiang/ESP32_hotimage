#include "IDW.h"

#define P0 (0.077847)
#define P1 (0.123317 + 0.077847)
#define P2 (0.195346 + 0.123317 + 0.123317 + 0.077847)

static int ktype = 2;

static const int8_t offset[4][9] = {
    { -33, -32, -32, -1, 0, 0, -1, 0, 0 },
    { -32, -32, -31, 0, 0, 1, 0, 0, 1 },
    { -1, 0, 0, -1, 0, 0, 31, 32, 32 },
    { 0, 0, 1, 0, 0, 1, 32, 32, 33 },
};

// const int8_t offset[4][9] =
//     {
//         {-65, -64, -64,
//          -2, 0, 0,
//          -2, 0, 0},
//         {-64, -64, -63,
//          0, 0, 2,
//          0, 0, 2},
//         {-2, 0, 0,
//          -2, 0, 0,
//          63, 64, 64},
//         {0, 0, 2,
//          0, 0, 2,
//          64, 64, 65},
// };

static const float kernel[3][9] = {
    // Signa = 0.5
    {
        0.024879f,
        0.107973f,
        0.024879f,
        0.107973f,
        0.468592f,
        0.107973f,
        0.024879f,
        0.107973f,
        0.024879f,
    },
    // Sigma = 1.0
    {
        0.077847f,
        0.123317f,
        0.077847f,
        0.123317f,
        0.195346f,
        0.123317f,
        0.077847f,
        0.123317f,
        0.077847f,
    },
    // Sigma = 2.0
    {
        0.102059f,
        0.115349f,
        0.102059f,
        0.115349f,
        0.130371f,
        0.115349f,
        0.102059f,
        0.115349f,
        0.102059f,
    },
};

/**
 * @brief 高斯模糊 2倍
 *
 * @param pSrc 输入
 * @param w 输入宽
 * @param h 输入高
 * @param pDest 输出
 */
void idwGauss(int16_t* pSrc, uint16_t w, uint16_t h, uint16_t scale, float* pDest)
{
    const float* pKernel = kernel[ktype];
    const int32_t totalSize = w * h;
    const uint16_t _Scale = scale * scale;

    for (int i = 0; i < totalSize * _Scale; i++) {
        float pix = 0;
        int sourceAddress = ((i >> 1) & 0x1f) + ((i & 0xffffff80) >> 2);
        uint16_t q = (i & 1) + ((i & 0x40) >> 5);

        for (int z = 0; z < 9; z++) {
            int sa = sourceAddress + offset[q][z];
            if (sa < 0) {
                sa = 0;
            } else if (sa >= totalSize) {
                sa = totalSize - 1;
            }
            pix += pKernel[z] * pSrc[sa];
        }
        pDest[i] = pix;
    }
}

#if 0

static const uint16_t kernelInt[3][9] = {
    {
        815,
        3538,
        815,
        3538,
        15355,
        3538,
        815,
        3538,
        815,
    },
    {
        2551,
        4041,
        2551,
        4041,
        6401,
        4041,
        2551,
        4041,
        2551,
    },
    {
        3344,
        3780,
        3344,
        3780,
        4272,
        3780,
        3344,
        3780,
        3344,
    }
};

/**
 * @brief 高斯插值 2倍 定点运算
 *
 * @param pSrc 输入
 * @param w 输入宽
 * @param h 输入高
 * @param dest
 */
void idwGaussInt(int16_t* pSrc, uint16_t w, uint16_t h, uint16_t scale, float* pDest)
{
    const uint16_t* pKernel = kernelInt[ktype];
    const int32_t totalSize = w * h;
    const uint16_t _Scale = scale * scale;

    for (int i = 0; i < totalSize * _Scale; i++) {
        float pix = 0;
        int sourceAddress = ((i >> 1) & 0x1f) + ((i & 0xffffff80) >> 2);
        uint16_t q = (i & 1) + ((i & 0x40) >> 5);

        for (int z = 0; z < 9; z++) {
            int sa = sourceAddress + offset[q][z];
            if (sa < 0) {
                sa = 0;
            } else if (sa >= totalSize) {
                sa = totalSize - 1;
            }
            pix += (pKernel[z] * (int32_t)(pSrc[sa])) >> 15;
        }
        pDest[i] = pix;
    }
}
#endif
