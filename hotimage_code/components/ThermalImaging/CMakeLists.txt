set(ThermalImaging_srcs   
    "src/console.c"   
    "src/func.c"
    "src/menu.c"
    "src/messagebox.c"
    "src/save.c"
    "src/settings.c"
    "src/sleep.c"
)

set(task_srcs
    "src/task/render_task.c" 
    "src/task/mlx90640_task.c" 
    "src/task/buttons_task.c"
    "src/task/sht31_task.c"
    "src/task/adc_task.c"
    "src/task/sd_task.c"
    "src/task/wifi_task.c"
)

set(lcd_srcs 
    "src/lcd/spi_lcd.c"
    "src/lcd/lcd_bl.c"
    "src/lcd/dispcolor.c"
    "src/lcd/st7789.c"
    "src/lcd/font/f32f.c"
    "src/lcd/font/f6x8m.c"
    "src/lcd/font/f16f.c"
    "src/lcd/font/f24f.c"
    "src/lcd/font/font.c"
)

set(iic_srcs
    "src/iic/iic.c"
    "src/iic/driver_MLX90640.c"
    "src/iic/driver_sht31.c"
    "src/iic/MLX90640_I2C_Driver.c"
)

set(interpolation_srcs
    "src/interpolation/Bilinear.c"
    "src/interpolation/Gauss.c"
    "src/interpolation/palette.c"
)

set(tools_srcs
    "src/tools/SAFiter.c"
    "src/tools/tools.c"
)

set(wifi_srcs
    "src/webserver/webserver.c"
    "src/webserver/fileserver.c"
)

idf_component_register(SRCS "${ThermalImaging_srcs}" "${lcd_srcs}" "${iic_srcs}" "${interpolation_srcs}" "${tools_srcs}" "${task_srcs}"  "${wifi_srcs}"
                       INCLUDE_DIRS "include" "include/lcd" "include/iic" "include/tasks" "include/tools" 
                       REQUIRES esp_adc_cal spi_flash nvs_flash fatfs esp_http_server)
