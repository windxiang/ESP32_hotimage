#include "MLX90640_I2C_Driver.h"
#include "iic.h"
#include <stdio.h>

#if 0
/**
 * @brief 读取字节数组的函数
 *
 * @param slaveAddr 设备地址
 * @param startAddress 寄存器地址
 * @param nBytesRead 读取大小
 * @param data 读取内存指针
 * @return int
 */
static int MLX90640_I2CReadBytes(uint8_t slaveAddr, uint16_t startAddress, uint16_t nBytesRead, uint8_t* data)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    // 首先，我们选择传感器的内部地址
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, slaveAddr << 1 | WRITE_BIT, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, startAddress >> 8, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, startAddress & 0xFF, ACK_CHECK_EN);

    // 现在我们从传感器读取
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, slaveAddr << 1 | READ_BIT, ACK_CHECK_EN);

    if (nBytesRead > 1)
        i2c_master_read(cmd, data, nBytesRead - 1, ACK_VAL);
    i2c_master_read_byte(cmd, data + nBytesRead - 1, NACK_VAL);

    i2c_master_stop(cmd);

    int ret = i2c_master_cmd_begin(I2C_NUM, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);

    return ret;
}
#endif

/**
 * @brief 读取2字节字数组的函数
 *
 * @param slaveAddr 设备地址
 * @param startAddress 寄存器地址
 * @param nMemAddressRead 读取大小
 * @param data 读取内存指针
 * @return int
 */
int MLX90640_I2CRead(uint8_t slaveAddr, uint16_t startAddress, uint16_t nMemAddressRead, uint16_t* data)
{
#if 1
    esp_err_t ret = i2c_master_read_slave_reg_16bit(I2C_NUM, slaveAddr, startAddress, (uint8_t*)data, nMemAddressRead << 1, 1000 / portTICK_RATE_MS);
    if (ret == ESP_OK) {
        uint16_t d;
        for (int i = 0; i < nMemAddressRead; i++) {
            d = data[i];
            data[i] = ((d & 0xFF) << 8) | ((d >> 8) & 0xFF);
        }
    }
    return ret;
#else
    uint8_t* mlxBuff = (uint8_t*)heap_caps_malloc(nMemAddressRead << 1, MALLOC_CAP_8BIT | MALLOC_CAP_DMA);
    int ret = MLX90640_I2CReadBytes(slaveAddr, startAddress, nMemAddressRead << 1, mlxBuff);
    if (ret < 0) {
        heap_caps_free(mlxBuff);
        return ret;
    }

    for (int cnt = 0, i = 0; cnt < nMemAddressRead; cnt++, i += 2)
        *(data++) = ((uint16_t)mlxBuff[i] << 8) + mlxBuff[i + 1];

    heap_caps_free(mlxBuff);

    return 0;
#endif
}

/**
 * @brief 2 字节字的写函数 i2c ESP32
 *
 * @param slaveAddr
 * @param writeAddress
 * @param data
 * @return int
 */
int MLX90640_I2CWrite(uint8_t slaveAddr, uint16_t writeAddress, uint16_t data)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    // 首先，我们选择传感器的内部地址
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (slaveAddr << 1) | WRITE_BIT, ACK_CHECK_EN);

    i2c_master_write_byte(cmd, writeAddress >> 8, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, writeAddress & 0xFF, ACK_CHECK_EN);

    // 数据
    i2c_master_write_byte(cmd, data >> 8, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, data & 0xFF, ACK_CHECK_EN);
    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(I2C_NUM, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);

    // 通过读取检查记录
    static uint16_t dataCheck;
    MLX90640_I2CRead(slaveAddr, writeAddress, 1, &dataCheck);

    if (dataCheck != data)
        return -2;

    return ret;
}
