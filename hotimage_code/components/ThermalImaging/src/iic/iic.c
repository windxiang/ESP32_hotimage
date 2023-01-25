#include "iic.h"

#ifdef CONFIG_ESP32_IIC_SUPPORT

/* 互斥信号量句柄 */
static SemaphoreHandle_t xSemaphore = NULL;

/**
 * @brief 初始化IIC设备
 *
 */
void I2CInit(int sda_io_num, int scl_io_num, uint32_t freqHZ, i2c_mode_t mode)
{
    i2c_config_t conf = {
        .mode = mode,
        .sda_io_num = sda_io_num,
        .scl_io_num = scl_io_num,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = freqHZ,
    };
    i2c_param_config(I2C_NUM, &conf);
    i2c_driver_install(I2C_NUM, conf.mode, I2C_RX_BUF_DISABLE, I2C_TX_BUF_DISABLE, 0);

    /* 创建互斥信号量 */
    xSemaphore = xSemaphoreCreateMutex();
}

/**
 * @brief IIC设备地址扫描
 *
 */
void i2c_master_scan(void)
{
    uint8_t address;
    printf("     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f\r\n");
    for (int i = 0; i < 128; i += 16) {
        printf("%02x: ", i);
        for (int j = 0; j < 16; j++) {
            fflush(stdout);
            address = i + j;

            i2c_cmd_handle_t cmd = i2c_cmd_link_create();
            i2c_master_start(cmd);
            i2c_master_write_byte(cmd, (address << 1) | WRITE_BIT, ACK_CHECK_EN);
            i2c_master_stop(cmd);

            esp_err_t ret = i2c_master_cmd_begin(I2C_NUM, cmd, 50 / portTICK_RATE_MS);
            i2c_cmd_link_delete(cmd);

            if (ret == ESP_OK) {
                printf("%02x ", address);
            } else if (ret == ESP_ERR_TIMEOUT) {
                printf("UU ");
            } else {
                printf("-- ");
            }
        }
        printf("\r\n");
    }

    i2c_driver_delete(I2C_NUM);
}

/**
 * @brief 总线上所有支持的地址将得到复位
 *        要支持此命令的设备才有效：目前已知设备 sht31
 *
 * @return esp_err_t
 */
esp_err_t i2c_general_reset()
{
    uint8_t data_wr = 0x06;
    return i2c_master_write_slave(I2C_NUM, 0x00, &data_wr, 1, 100 / portTICK_RATE_MS);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#if 0
uint8_t iic_write_address16(uint8_t addr, uint16_t reg, uint8_t* pBuf, uint16_t len)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    // 1 选择传感器的内部地址
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (addr << 1) | WRITE_BIT, ACK_CHECK_EN); // 地址
    i2c_master_write_byte(cmd, (reg >> 8) & 0xFF, ACK_CHECK_EN); // 命令 MSB
    i2c_master_write_byte(cmd, reg & 0xFF, ACK_CHECK_EN); // 命令 LSB

    // 2 写入内容
    if (len > 0 && NULL != pBuf) {
        // TODO 后面在扩展
    }

    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(I2C_NUM, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);

    return ret;
}

uint8_t iic_read_address16(uint8_t addr, uint16_t reg, uint8_t* pBuf, uint16_t len)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    // 1 选择传感器的内部地址
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (addr << 1) | WRITE_BIT, ACK_CHECK_EN); // 地址
    i2c_master_write_byte(cmd, (reg >> 8) & 0xFF, ACK_CHECK_EN); // 命令 MSB
    i2c_master_write_byte(cmd, reg & 0xFF, ACK_CHECK_EN); // 命令 LSB

    // 2 读取数据
    if (len > 0 && NULL != pBuf) {
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_READ, ACK_CHECK_EN); // 地址
        i2c_master_read(cmd, pBuf, len, I2C_MASTER_LAST_NACK);
    }
    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(I2C_NUM, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);

    return ret;
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
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
esp_err_t i2c_master_read_slave(i2c_port_t i2c_num, uint8_t slave_addr, uint8_t* data_rd, size_t size, TickType_t ticks_to_wait)
{
    if (size == 0) {
        return ESP_OK;
    }

    xSemaphoreTake(xSemaphore, portMAX_DELAY);

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (slave_addr << 1) | I2C_MASTER_READ, ACK_CHECK_EN); // 设备地址

    if (size > 1) {
        i2c_master_read(cmd, data_rd, size - 1, ACK_VAL); // 除了最后一个字节
    }
    i2c_master_read_byte(cmd, data_rd + size - 1, NACK_VAL); // 最后一个字节

    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(i2c_num, cmd, ticks_to_wait);
    i2c_cmd_link_delete(cmd);

    xSemaphoreGive(xSemaphore);
    return ret;
}

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
esp_err_t i2c_master_read_slave_reg(i2c_port_t i2c_num, uint8_t slave_addr, uint8_t reg_addr, uint8_t* data_rd, size_t size, TickType_t ticks_to_wait)
{
    if (size == 0) {
        return ESP_OK;
    }

    xSemaphoreTake(xSemaphore, portMAX_DELAY);

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (slave_addr << 1) | I2C_MASTER_WRITE, ACK_CHECK_EN); // 设备地址
    i2c_master_write_byte(cmd, reg_addr, ACK_CHECK_EN);

    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (slave_addr << 1) | I2C_MASTER_READ, ACK_CHECK_EN);

    if (size > 1) {
        i2c_master_read(cmd, data_rd, size - 1, ACK_VAL); // 除了最后一个字节
    }
    i2c_master_read_byte(cmd, data_rd + size - 1, NACK_VAL); // 最后一个字节

    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(i2c_num, cmd, ticks_to_wait);
    i2c_cmd_link_delete(cmd);

    xSemaphoreGive(xSemaphore);
    return ret;
}

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
esp_err_t i2c_master_read_slave_reg_16bit(i2c_port_t i2c_num, uint8_t slave_addr, uint16_t reg_addr, uint8_t* data_rd, size_t size, TickType_t ticks_to_wait)
{
    if (size == 0) {
        return ESP_OK;
    }

    xSemaphoreTake(xSemaphore, portMAX_DELAY);

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (slave_addr << 1) | I2C_MASTER_WRITE, ACK_CHECK_EN); // 设备地址 写入模式
    i2c_master_write_byte(cmd, reg_addr >> 8, ACK_CHECK_EN); // 寄存器地址
    i2c_master_write_byte(cmd, reg_addr & 0xFF, ACK_CHECK_EN); // 寄存器地址

    // 重新Start
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (slave_addr << 1) | I2C_MASTER_READ, ACK_CHECK_EN); // 读取模式
    if (size > 1) {
        i2c_master_read(cmd, data_rd, size - 1, ACK_VAL); // 连续读取
    }
    i2c_master_read_byte(cmd, data_rd + size - 1, NACK_VAL); // 最后一个字节

    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(i2c_num, cmd, ticks_to_wait);
    i2c_cmd_link_delete(cmd);

    xSemaphoreGive(xSemaphore);
    return ret;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
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
esp_err_t i2c_master_write_slave(i2c_port_t i2c_num, uint8_t slave_addr, uint8_t* data_wr, size_t size, TickType_t ticks_to_wait)
{
    if (size == 0) {
        return ESP_OK;
    }

    xSemaphoreTake(xSemaphore, portMAX_DELAY);

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (slave_addr << 1) | I2C_MASTER_WRITE, ACK_CHECK_EN); // 设备地址

    i2c_master_write(cmd, data_wr, size, ACK_CHECK_EN); // 写入数据
    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(i2c_num, cmd, ticks_to_wait);
    i2c_cmd_link_delete(cmd);

    xSemaphoreGive(xSemaphore);
    return ret;
}

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
esp_err_t i2c_master_write_slave_reg(i2c_port_t i2c_num, uint8_t slave_addr, uint8_t reg_addr, uint8_t* data_wr, size_t size, TickType_t ticks_to_wait)
{
    if (size == 0) {
        return ESP_OK;
    }
    xSemaphoreTake(xSemaphore, portMAX_DELAY);

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (slave_addr << 1) | I2C_MASTER_WRITE, ACK_CHECK_EN); // 设备地址

    i2c_master_write_byte(cmd, reg_addr, ACK_CHECK_EN); // 寄存器地址
    i2c_master_write(cmd, data_wr, size, ACK_CHECK_EN); // 数据
    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(i2c_num, cmd, ticks_to_wait);
    i2c_cmd_link_delete(cmd);

    xSemaphoreGive(xSemaphore);
    return ret;
}

#endif // CONFIG_ESP32_IIC_SUPPORT