#ifndef MAIN_SD_SD_H_
#define MAIN_SD_SD_H_

#include "esp_system.h"

uint8_t sdcardIsMount();
void sdcard_task(void* arg);

#endif /* MAIN_SD_SD_H_ */
