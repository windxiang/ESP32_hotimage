#ifndef __IDW_H
#define __IDW_H

#include <math.h>
#include <stdint.h>
#include <stdlib.h>

void idwBilinear(const float* pSrcGaussBuf, uint8_t src_gauss_width, uint8_t src_gauss_height, int16_t* pDest, uint16_t dest_width, uint16_t dest_height, uint8_t upScaleFactor);
void idwOldInterpolate(int16_t* pSrcImage, uint16_t srcWidth, uint16_t srcHeight, uint16_t scale, int16_t* pHDImageOut);

void idwGauss(int16_t* pSrc, uint16_t w, uint16_t h, uint16_t scale, float* pDest);
// void idwGaussInt(int16_t* pSrc, uint16_t w, uint16_t h, uint16_t scale, float* pDest);

#endif
