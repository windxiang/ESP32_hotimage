#ifndef _THERMALIMAGING_H
#define _THERMALIMAGING_H

#include "esp_system.h"

#include "IDW.h"

#include "console.h"
#include "func.h"
#include "menu.h"
#include "messagebox.h"
#include "palette.h"
#include "save.h"
#include "settings.h"
#include "sleep.h"

// lcd
#include "dispcolor.h"
#include "f16f.h"
#include "f24f.h"
#include "f32f.h"
#include "f6x8m.h"
#include "font.h"
#include "st7789.h"

// IIC
#include "MLX90640_I2C_Driver.h"
#include "driver_MLX90640.h"
#include "driver_sht31.h"
#include "iic.h"

// task
#include "adc_task.h"
#include "buttons_task.h"
#include "mlx90640_task.h"
#include "render_task.h"
#include "sd_task.h"
#include "sht31_task.h"
#include "wifi_task.h"

// tools
#include "SAFiter.h"
#include "tools.h"

#endif // _THERMALIMAGING_H