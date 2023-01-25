#ifndef MAIN_PALETTE_PALETTE_H_
#define MAIN_PALETTE_PALETTE_H_

#include "settings.h"

typedef struct
{
	uint8_t r;
	uint8_t g;
	uint8_t b;
} tRGBcolor;

// 将指针返回到所选类型的伪彩色点数组
void getPalette(eColorScale palette, uint16_t steps, tRGBcolor *pBuff);


#endif /* MAIN_PALETTE_PALETTE_H_ */
