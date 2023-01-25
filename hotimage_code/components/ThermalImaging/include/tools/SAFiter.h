#ifndef _SA_FITER_H_
#define _SA_FITER_H_

#include <stdio.h>

/*滑动平均滤波器——结构体*/
typedef struct SlipAveFilter {
    uint16_t len; // 窗口宽度
    uint16_t index; // 索引
    uint16_t has; // 已有的数据个数
    char isfull; /*数组数据是否已满*/
    float* data; /*滤波器数组*/
    float sum; /*求和*/
    float res; /*滤波结果*/
} SAFilterHandle_t;

SAFilterHandle_t* SlipAveFilterCreate(uint16_t len);
float AddSAFiterRes(SAFilterHandle_t* pFilter, float input);
float GetSAFiterRes(SAFilterHandle_t* pFilter);

#endif /* _SA_FITER_H_ */