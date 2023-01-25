#ifndef MAIN_SAVE_SAVE_H_
#define MAIN_SAVE_SAVE_H_
#include "esp_system.h"

int save_ImageCSV(void);
int save_ImageBMP(uint8_t bits);
int save_MLX90640Params(void);

#endif /* MAIN_SAVE_SAVE_H_ */
