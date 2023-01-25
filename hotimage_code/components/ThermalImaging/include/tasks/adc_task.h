#ifndef MAIN_ADC_ADC_H_
#define MAIN_ADC_ADC_H_

#include <stdio.h>

uint32_t getBatteryVoltage();
int8_t getBatteryCharge();
void adc_task(void* arg);


#endif /* MAIN_ADC_ADC_H_ */
