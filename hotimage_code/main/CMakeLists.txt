idf_component_register(SRCS "app_main.c" 
                       INCLUDE_DIRS "." 
                       REQUIRES esp_hw_support spi_flash ThermalImaging)
