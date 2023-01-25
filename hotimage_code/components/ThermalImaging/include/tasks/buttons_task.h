#ifndef MAIN_TASK_BUTTONS_H_
#define MAIN_TASK_BUTTONS_H_


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"

void buttons_init();
void buttons_task(void* arg);


#endif /* MAIN_TASK_BUTTONS_H_ */
