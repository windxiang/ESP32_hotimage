#ifndef _IIC_H_
#define _IIC_H_

#include "esp_system.h"
#include <driver/i2c.h>

// ESP32 IIC 使用接口定义
#define I2C_NUM CONFIG_ESP32_IIC_NUM /*!< I2C port number for master dev */

#define I2C_TX_BUF_DISABLE 0 /*!< I2C master do not need buffer */
#define I2C_RX_BUF_DISABLE 0 /*!< I2C master do not need buffer */

#define WRITE_BIT I2C_MASTER_WRITE /*!< I2C master write */
#define READ_BIT I2C_MASTER_READ /*!< I2C master read */

#define ACK_CHECK_EN 1 /*!< I2C master will check ack from slave*/
#define ACK_CHECK_DIS 0 /*!< I2C master will not check ack from slave */

#define ACK_VAL (i2c_ack_type_t)0 /*!< I2C ack value */
#define NACK_VAL (i2c_ack_type_t)1 /*!< I2C nack value */

/**
 * @brief 初始化IIC设备
 *
 */
void I2CInit(int sda_io_num, int scl_io_num, uint32_t freqHZ, i2c_mode_t mode);

/**
 * @brief IIC设备地址扫描
 *
 */
void i2c_master_scan(void);

/**
 * @brief 总线上所有支持的地址将得到复位
 *        要支持此命令的设备才有效：目前已知设备 sht31
 *
 * @return esp_err_t
 */
esp_err_t i2c_general_reset();

/**
 * @brief  I2Cx-读从设备的值
 *      - 不带有读器件寄存器的方式，适用于 BH1750、ADS1115/1118等少数I2C设备，这类设备通常内部寄存器很少
 *      - 例：i2c_master_read_slave(I2C_NUM_0, 0x68, &test, 1, 100 / portTICK_RATE_MS);
 *
 * ________________________________________________________________________________________
 * | start | slave_addr + rd_bit + ack | read n-1 bytes + ack | read 1 byte + nack | stop |
 * --------|---------------------------|----------------------|--------------------|------|
 *
 */
esp_err_t i2c_master_read_slave(i2c_port_t i2c_num, uint8_t slave_addr, uint8_t* data_rd, size_t size, TickType_t ticks_to_wait);

/**
 * @brief  I2Cx-读从设备的寄存器值
 *      - 带有读器件寄存器的方式，适用于 MPU6050、ADXL345、HMC5983、MS5611、BMP280等绝大多数I2C设备
 *      - 例：i2c_master_read_slave_reg(I2C_NUM_0, 0x68, 0x75, &test, 1, 100 / portTICK_RATE_MS);
 *
 * _____________________________________________________________________________________________________________________________________________
 * | start | slave_addr + rd_bit + ack | reg_addr + ack | start | slave_addr + wr_bit + ack | read n-1 bytes + ack | read 1 byte + nack | stop |
 * --------|---------------------------|------------------------|---------------------------|----------------------|--------------------|------|
 *
 * @param  i2c_num I2C端口号。I2C_NUM_0 / I2C_NUM_1
 * @param  slave_addr I2C读从机的器件地址
 * @param  reg_addr I2C读从机的寄存器地址
 * @param  data_rd 读出的值的指针，存放读取出的数据
 * @param  size 读取的寄存器数目
 * @param  ticks_to_wait 超时等待时间
 *
 * @return
 *     - esp_err_t
 */
esp_err_t i2c_master_read_slave_reg(i2c_port_t i2c_num, uint8_t slave_addr, uint8_t reg_addr, uint8_t* data_rd, size_t size, TickType_t ticks_to_wait);

/**
 * @brief  I2Cx-读从设备的寄存器值（寄存器地址 或 命令 为2字节的器件）
 *      - 带有读器件寄存器的方式，适用于 SHT20、GT911 这种寄存器地址为16位的I2C设备
 *      - 例：i2c_master_read_slave_reg_16bit(I2C_NUM_0, 0x44, 0xE000, &test, 6, 100 / portTICK_RATE_MS);
 *
 * ____________________________________________________________________________________________________________________________________________________
 * | start | slave_addr + rd_bit + ack | reg_addr(2byte) + ack | start | slave_addr + wr_bit + ack | read n-1 bytes + ack | read 1 byte + nack | stop |
 * --------|---------------------------|-------------------------------|---------------------------|----------------------|--------------------|------|
 *
 * @param  i2c_num I2C端口号。I2C_NUM_0 / I2C_NUM_1
 * @param  slave_addr I2C读从机的器件地址
 * @param  reg_addr I2C读从机的寄存器地址(2byte)
 * @param  data_rd 读出的值的指针，存放读取出的数据
 * @param  size 读取的寄存器数目
 * @param  ticks_to_wait 超时等待时间
 *
 * @return
 *     - esp_err_t
 */
esp_err_t i2c_master_read_slave_reg_16bit(i2c_port_t i2c_num, uint8_t slave_addr, uint16_t reg_addr, uint8_t* data_rd, size_t size, TickType_t ticks_to_wait);

/**
 * @brief  I2Cx-写从设备的值
 *      - 不带有写器件寄存器的方式，适用于 BH1750、ADS1115/1118等少数I2C设备，这类设备通常内部寄存器很少
 *      - 例：i2c_master_write_slave(I2C_NUM_0, 0x68, &test, 1, 100 / portTICK_RATE_MS);
 *
 * ___________________________________________________________________
 * | start | slave_addr + wr_bit + ack | write n bytes + ack  | stop |
 * --------|---------------------------|----------------------|------|
 *
 */
esp_err_t i2c_master_write_slave(i2c_port_t i2c_num, uint8_t slave_addr, uint8_t* data_wr, size_t size, TickType_t ticks_to_wait);

/**
 * @brief  I2Cx-写从设备的寄存器值
 *      - 带有写器件寄存器的方式，适用于 MPU6050、ADXL345、HMC5983、MS5611、BMP280等绝大多数I2C设备
 *      - 例：i2c_master_write_slave_reg(I2C_NUM_0, 0x68, 0x75, &test, 1, 100 / portTICK_RATE_MS);
 *
 * ____________________________________________________________________________________
 * | start | slave_addr + wr_bit + ack | reg_addr + ack | write n bytes + ack  | stop |
 * --------|---------------------------|----------------|----------------------|------|
 *
 * @param  i2c_num I2C端口号。I2C_NUM_0 / I2C_NUM_1
 * @param  slave_addr I2C写从机的器件地址
 * @param  reg_addr I2C写从机的寄存器地址
 * @param  data_wr 写入的值的指针，存放写入进的数据
 * @param  size 写入的寄存器数目
 * @param  ticks_to_wait 超时等待时间
 *
 * @return
 *     - esp_err_t
 */
esp_err_t i2c_master_write_slave_reg(i2c_port_t i2c_num, uint8_t slave_addr, uint8_t reg_addr, uint8_t* data_wr, size_t size, TickType_t ticks_to_wait);

#endif /* _IIC_H_ */
