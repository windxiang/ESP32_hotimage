#include "driver_sht31.h"
#include "iic.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief 芯片命令支持
 */
#define SHT31_COMMAND_FETCH_DATA 0xE000U /**< fetch data command */
#define SHT31_COMMAND_ART 0x2B32U /**< art command */
#define SHT31_COMMAND_BREAK 0x3093U /**< break command */
#define SHT31_COMMAND_SOFT_RESET 0x30A2U /**< soft reset command */
#define SHT31_COMMAND_HEATER_ENABLE 0x306DU /**< heater enable command */
#define SHT31_COMMAND_HEATER_DISABLE 0x3066U /**< heater disable command */
#define SHT31_COMMAND_READ_STATUS 0xF32DU /**< read status command */
#define SHT31_COMMAND_CLEAR_STATUS 0x3041U /**< clear status command */

static uint8_t sht31_iic_addr = 0x44; // SHT31 IIC 地址
static sht31_handle_t sht31_handle = { 0 };

static void debug_print(const char* const fmt, ...)
{
    char StrBuff[256] = { 0 };
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(StrBuff, sizeof(StrBuff), fmt, ap);
    va_end(ap);

    printf("%s", StrBuff);
}

/**
 * @brief     设置寄存器
 * @param[in] *handle points to a sht31 handle structure
 * @param[in] reg 寄存器地址
 * @return    status code
 *            - 0 success
 *            - 1 write failed
 * @note      none
 */
static uint8_t a_sht31_write(uint16_t reg)
{
    // return iic_write_address16(sht31_iic_addr, reg, NULL, 0);
    uint8_t cmd_buffer[2];
    cmd_buffer[0] = (reg >> 8) & 0xFF;
    cmd_buffer[1] = reg & 0xFF;
    return i2c_master_write_slave_reg(I2C_NUM, sht31_iic_addr, cmd_buffer[0], cmd_buffer + 1, 1, 100 / portTICK_RATE_MS);
}

/**
 * @brief      读取数据
 * @param[in]  *handle points to a sht31 handle structure
 * @param[in]  reg 寄存器地址
 * @param[out] *data 数据缓存
 * @param[in]  len 数据长度
 * @return     status code
 *             - 0 success
 *             - 1 read failed
 * @note       none
 */
static uint8_t a_sht31_read(uint16_t reg, uint8_t* data, uint16_t len)
{
    return i2c_master_read_slave_reg_16bit(I2C_NUM, sht31_iic_addr, reg, data, len, 100 / portTICK_RATE_MS);
}

/**
 * @brief      读取数据 不带寄存器地址
 * @param[in]  *handle points to a sht31 handle structure
 * @param[out] *data 数据缓存
 * @param[in]  len 数据长度
 * @return     status code
 *             - 0 success
 *             - 1 read failed
 * @note       none
 */
static uint8_t a_sht31_read_noreg(uint8_t* data, uint16_t len)
{
    return i2c_master_read_slave(I2C_NUM, sht31_iic_addr, data, len, 100 / portTICK_RATE_MS);
}

/**
 * @brief     计算crc
 * @param[in] *data points to a data buffer
 * @param[in] len is the data length
 * @return    crc
 * @note      none
 */
static uint8_t a_sht31_crc(uint8_t* data, uint16_t len)
{
    const uint8_t POLYNOMIAL = 0x31;
    uint8_t crc = 0xFF;
    uint16_t i, j;

    for (j = len; j != 0; --j) /* length-- */
    {
        crc ^= *data++; /* xor */
        for (i = 8; i != 0; --i) /* 8 times */
        {
            crc = (crc & 0x80) ? (crc << 1) ^ POLYNOMIAL : (crc << 1); /* calculate crc */
        }
    }

    return crc; /* return crc */
}

/**
 * @brief     初始化芯片
 * @return    status code
 *            - 0 success
 *            - 1 iic initialization failed
 *            - 3 linked functions is NULL
 * @note      none
 */
uint8_t sht31_init()
{
    sht31_handle.repeatability = SHT31_REPEATABILITY_MEDIUM;
    sht31_handle.inited = 1; /* flag finish initialization */
    return 0;
}

/**
 * @brief     关闭芯片
 * @return    status code
 *            - 0 success
 *            - 1 iic deinit failed
 *            - 3 handle is not initialized
 * @note      none
 */
uint8_t sht31_deinit()
{
    uint8_t res;

    if (sht31_handle.inited != 1) /* check handle initialization */
    {
        return 3; /* return error */
    }

    res = a_sht31_write(SHT31_COMMAND_BREAK); /* write command */
    if (res != 0) /* check result */
    {
        debug_print("sht31: write command failed.\n"); /* write command failed */
        return 1; /* return error */
    }
    sht31_handle.inited = 0; /* flag close */

    return ESP_OK; /* success return 0 */
}

/**
 * @brief      get the current status
 * @param[out] *status points to a status buffer
 * @return      status code
 *              - 0 success
 *              - 1 get status failed
 * @note        none
 */
uint8_t sht31_get_status(uint16_t* status)
{
    uint8_t res;
    uint8_t data[3];

    if (sht31_handle.inited != 1) /* check handle initialization */
    {
        return 3; /* return error */
    }

    memset(data, 0, sizeof(uint8_t) * 3); /* clear the buffer */
    res = a_sht31_read(SHT31_COMMAND_READ_STATUS, (uint8_t*)data, 3); /* read status */
    if (res != 0) {
        /* check result */
        debug_print("sht31: read status failed.\n"); /* read status failed */
        return 1; /* return error */
    }

    if (a_sht31_crc((uint8_t*)data, 2) == data[2]) {
        /* check crc */
        *status = (uint16_t)((((uint16_t)data[0]) << 8) | data[1]); /* get status */
        return 0; /* success return 0 */

    } else {
        debug_print("sht31: crc check failed.\n"); /* crc check failed */
        return 1; /* return error */
    }
}

/**
 * @brief     clear the current status
 * @return    status code
 *            - 0 success
 *            - 1 clear status failed
 * @note      none
 */
uint8_t sht31_clear_status()
{
    uint8_t res;

    if (sht31_handle.inited != 1) /* check handle initialization */
    {
        return 3; /* return error */
    }

    res = a_sht31_write(SHT31_COMMAND_CLEAR_STATUS); /* write command */
    if (res != 0) {
        /* check result */
        debug_print("sht31: write command failed.\n"); /* write command failed */
        return 1; /* return error */
    }

    return 0; /* success return 0 */
}

/**
 * @brief     set the measurement repeatability
 * @param[in] repeatability is the measurement repeatability
 * @return    status code
 *            - 0 success
 *            - 1 set repeatability failed
 * @note      none
 */
uint8_t sht31_set_repeatability(sht31_repeatability_t repeatability)
{
    sht31_handle.repeatability = (uint8_t)repeatability; /* set repeatability */
    return 0; /* success return 0 */
}

/**
 * @brief      get the measurement repeatability
 * @param[out] *repeatability points to a measurement repeatability buffer
 * @return     status code
 *             - 0 success
 *             - 1 get repeatability failed
 * @note       none
 */
uint8_t sht31_get_repeatability(sht31_repeatability_t* repeatability)
{
    *repeatability = (sht31_repeatability_t)(sht31_handle.repeatability); /* get repeatability */
    return 0; /* success return 0 */
}

/**
 * @brief      启动单次测量模式 发送命令
 *             然后调用 sht31_single_read 函数来读取数据
 * @param[in]  clock_stretching_enable is a clock stretching bool value
 * @param[out] *temperature_raw 原始温度缓冲区
 * @param[out] *temperature_s 转换后的温度缓冲区
 * @param[out] *humidity_raw 原始湿度缓冲区
 * @param[out] *humidity_s 转换后的湿度缓冲区
 * @return     status code
 *             - 0 success
 *             - 1 single read failed
 *             - 3 handle is not initialized
 * @note       none
 */
uint8_t sht31_single_command(sht31_bool_t clock_stretching_enable)
{
    uint8_t res;
    uint16_t command;

    if (sht31_handle.inited != 1) /* check handle initialization */
    {
        return 3; /* return error */
    }

    if (sht31_handle.repeatability == SHT31_REPEATABILITY_HIGH) {
        /* repeatability high */
        if (clock_stretching_enable == SHT31_BOOL_FALSE) {
            /* if disable clock stretching */
            command = 0x2400U; /* set disable high */
        } else if (clock_stretching_enable == SHT31_BOOL_TRUE) {
            /* if enable clock stretching */
            command = 0x2C06U; /* set enable high */
        } else {
            debug_print("sht31: clock stretching is invalid.\n"); /* clock stretching is invalid */
            return 1; /* return error */
        }

    } else if (sht31_handle.repeatability == SHT31_REPEATABILITY_MEDIUM) {
        /* repeatability medium */
        if (clock_stretching_enable == SHT31_BOOL_FALSE) {
            /* if disable clock stretching */
            command = 0x240BU; /* set disable medium */
        } else if (clock_stretching_enable == SHT31_BOOL_TRUE) {
            /* if enable clock stretching */
            command = 0x2C0DU; /* set enable medium */
        } else {
            debug_print("sht31: clock stretching is invalid.\n"); /* clock stretching is invalid */
            return 1; /* return error */
        }

    } else if (sht31_handle.repeatability == SHT31_REPEATABILITY_LOW) {
        /* repeatability low */
        if (clock_stretching_enable == SHT31_BOOL_FALSE) {
            /* if disable clock stretching */
            command = 0x2416U; /* set disable low */
        } else if (clock_stretching_enable == SHT31_BOOL_TRUE) {
            /* if enable clock stretching */
            command = 0x2C10U; /* set enable low */
        } else {
            debug_print("sht31: clock stretching is invalid.\n"); /* clock stretching is invalid */
            return 1; /* return error */
        }

    } else {
        debug_print("sht31: repeatability is invalid.\n"); /* repeatability is invalid */
        return 1; /* return error */
    }

    res = a_sht31_write(command); /* read data */
    if (res != 0) {
        /* check result */
        debug_print("sht31: read data failed.\n"); /* read data failed */
        return 1; /* return error */
    }

    return 0; /* success return 0 */
}

/**
 * @brief      启动单次测量模式
 *             先要调用 sht31_single_command 函数来触发采集数据后, 才可以调用读取
 * @param[in]  clock_stretching_enable is a clock stretching bool value
 * @param[out] *temperature_raw 原始温度缓冲区
 * @param[out] *temperature_s 转换后的温度缓冲区
 * @param[out] *humidity_raw 原始湿度缓冲区
 * @param[out] *humidity_s 转换后的湿度缓冲区
 * @return     status code
 *             - 0 success
 *             - 1 single read failed
 *             - 3 handle is not initialized
 * @note       none
 */
uint8_t sht31_single_read(uint16_t* temperature_raw, float* temperature_s, uint16_t* humidity_raw, float* humidity_s)
{
    uint8_t res;
    uint8_t data[6];

    if (sht31_handle.inited != 1) /* check handle initialization */
    {
        return 3; /* return error */
    }

    memset(data, 0, sizeof(uint8_t) * 6); /* clear the buffer */
    res = a_sht31_read_noreg((uint8_t*)data, 6); /* read data */
    if (res != 0) {
        /* check result */
        debug_print("sht31: read data failed.\n"); /* read data failed */
        return 1; /* return error */
    }

    if (a_sht31_crc((uint8_t*)data, 2) != data[2]) {
        /* check crc */
        debug_print("sht31: crc check failed.\n"); /* crc check failed */
        return 1; /* return error */
    }

    if (a_sht31_crc((uint8_t*)&data[3], 2) != data[5]) {
        /* check crc */
        debug_print("sht31: crc check failed.\n"); /* crc check failed */
        return 1; /* return error */
    }

    if (NULL != temperature_raw) {
        *temperature_raw = (uint16_t)((((uint16_t)data[0]) << 8) | data[1]); /* get raw temperature */
    }
    if (NULL != humidity_raw) {
        *humidity_raw = (uint16_t)((((uint16_t)data[3]) << 8) | data[4]); /* get raw humidity */
    }
    if (NULL != temperature_s) {
        *temperature_s = (float)(*temperature_raw) / 65535.0f * 175.0f - 45.0f; /* convert raw temperature */
    }
    if (NULL != humidity_s) {
        *humidity_s = (float)(*humidity_raw) / 65535.0f * 100.0f; /* convert raw humidity */
    }

    return 0; /* success return 0 */
}

/**
 * @brief     启动连续测量模式
 * @param[in] rate is the sample rate
 * @return    status code
 *            - 0 success
 *            - 1 start continuous read failed
 *            - 3 handle is not initialized
 * @note      none
 */
uint8_t sht31_start_continuous_read(sht31_rate_t rate)
{
    uint8_t res;
    uint16_t command;

    if (sht31_handle.inited != 1) /* check handle initialization */
    {
        return 3; /* return error */
    }

    if (sht31_handle.repeatability == SHT31_REPEATABILITY_HIGH) {
        /* repeatability high */
        if (rate == SHT31_RATE_0P5HZ) {
            /* 0.5Hz */
            command = 0x2032U; /* set 0.5Hz high */
        } else if (rate == SHT31_RATE_1HZ) { /* 1Hz */
            command = 0x2130U; /* set 1Hz high */
        } else if (rate == SHT31_RATE_2HZ) {
            /* 2Hz */
            command = 0x2236U; /* set 2Hz high */
        } else if (rate == SHT31_RATE_4HZ) {
            /* 4Hz */
            command = 0x2334U; /* set 4Hz high */
        } else if (rate == SHT31_RATE_10HZ) {
            /* 10Hz */
            command = 0x2737U; /* set 10Hz high */
        } else {
            debug_print("sht31: rate is invalid.\n"); /* rate is invalid */
            return 1; /* return error */
        }

    } else if (sht31_handle.repeatability == SHT31_REPEATABILITY_MEDIUM) {
        /* repeatability medium */
        if (rate == SHT31_RATE_0P5HZ) {
            /* 0.5Hz */
            command = 0x2024U; /* set 0.5Hz medium */
        } else if (rate == SHT31_RATE_1HZ) {
            /* 1Hz */
            command = 0x2126U; /* set 1Hz medium */
        } else if (rate == SHT31_RATE_2HZ) {
            /* 2Hz */
            command = 0x2220U; /* set 2Hz medium */
        } else if (rate == SHT31_RATE_4HZ) {
            /* 4Hz */
            command = 0x2322U; /* set 4Hz medium */
        } else if (rate == SHT31_RATE_10HZ) {
            /* 10Hz */
            command = 0x2721U; /* set 10Hz medium */
        } else {
            debug_print("sht31: rate is invalid.\n"); /* rate is invalid */
            return 1; /* return error */
        }

    } else if (sht31_handle.repeatability == SHT31_REPEATABILITY_LOW) {
        /* repeatability low */
        if (rate == SHT31_RATE_0P5HZ) {
            /* 0.5Hz */
            command = 0x202FU; /* set 0.5Hz low */
        } else if (rate == SHT31_RATE_1HZ) {
            /* 1Hz */
            command = 0x212DU; /* set 1Hz low */
        } else if (rate == SHT31_RATE_2HZ) {
            /* 2Hz */
            command = 0x222BU; /* set 2Hz low */
        } else if (rate == SHT31_RATE_4HZ) {
            /* set 4Hz */
            command = 0x2329U; /* set 4Hz low */
        } else if (rate == SHT31_RATE_10HZ) {
            /* 10Hz */
            command = 0x272AU; /* set 10Hz low */
        } else {
            debug_print("sht31: rate is invalid.\n"); /* rate is invalid */
            return 1; /* return error */
        }

    } else {
        debug_print("sht31: repeatability is invalid.\n"); /* repeatability is invalid */
        return 1; /* return error */
    }

    res = a_sht31_write(command); /* write command */
    if (res != 0) /* check result */
    {
        debug_print("sht31: write command failed.\n"); /* write command failed */
        return 1; /* return error */
    }
    return 0; /* success return 0 */
}

/**
 * @brief     停止连续测量模式
 * @return    status code
 *            - 0 success
 *            - 1 stop continuous read failed
 *            - 3 handle is not initialized
 * @note      none
 */
uint8_t sht31_stop_continuous_read()
{
    uint8_t res;

    if (sht31_handle.inited != 1) /* check handle initialization */
    {
        return 3; /* return error */
    }

    res = a_sht31_write(SHT31_COMMAND_BREAK); /* write command */
    if (res != 0) /* check result */
    {
        debug_print("sht31: write command failed.\n"); /* write command failed */
        return 1; /* return error */
    }

    return 0; /* success return 0 */
}

/**
 * @brief      连续测量模式下 读取数据
 * @param[out] *temperature_raw 一个原始温度缓冲区
 * @param[out] *temperature_s 转换后的温度缓冲区
 * @param[out] *humidity_raw 原始湿度缓冲区
 * @param[out] *humidity_s 转换后的湿度缓冲区
 * @return     status code
 *             - 0 success
 *             - 1 continuous read failed
 *             - 3 handle is not initialized
 * @note       none
 */
uint8_t sht31_continuous_read(uint16_t* temperature_raw, float* temperature_s, uint16_t* humidity_raw, float* humidity_s)
{
    uint8_t res;
    uint8_t data[6];

    if (sht31_handle.inited != 1) /* check handle initialization */
    {
        return 3; /* return error */
    }

    res = a_sht31_read(SHT31_COMMAND_FETCH_DATA, (uint8_t*)data, 6); /* read data */
    if (res != 0) /* check result */
    {
        debug_print("sht31: read data failed.\n"); /* read data failed */
        return 1; /* return error */
    }

    if (a_sht31_crc((uint8_t*)data, 2) != data[2]) /* check crc */
    {
        debug_print("sht31: crc check failed.\n"); /* crc check failed */
        return 1; /* return error */
    }

    if (a_sht31_crc((uint8_t*)&data[3], 2) != data[5]) /* check crc */
    {
        debug_print("sht31: crc check failed.\n"); /* crc check failed */
        return 1; /* return error */
    }

    if (NULL != temperature_raw) {
        *temperature_raw = (uint16_t)((((uint16_t)data[0]) << 8) | data[1]); /* get raw temperature */
    }
    if (NULL != humidity_raw) {
        *humidity_raw = (uint16_t)((((uint16_t)data[3]) << 8) | data[4]); /* get raw humidity */
    }

    if (NULL != temperature_s) {
        *temperature_s = (float)(*temperature_raw) / 65535.0f * 175.0f - 45.0f; /* convert raw temperature */
    }
    if (NULL != humidity_s) {
        *humidity_s = (float)(*humidity_raw) / 65535.0f * 100.0f; /* convert raw humidity */
    }

    return 0; /* success return 0 */
}

/**
 * @brief     set the chip art
 * @return    status code
 *            - 0 success
 *            - 1 set art failed
 *            - 3 handle is not initialized
 * @note      none
 */
uint8_t sht31_set_art()
{
    uint8_t res;

    if (sht31_handle.inited != 1) /* check handle initialization */
    {
        return 3; /* return error */
    }

    res = a_sht31_write(SHT31_COMMAND_ART); /* write command */
    if (res != 0) /* check result */
    {
        debug_print("sht31: write 0x%x command failed.\n", SHT31_COMMAND_ART); /* write command failed */
        return 1; /* return error */
    }

    return 0; /* success return 0 */
}

/**
 * @brief     复位芯片
 * @return    status code
 *            - 0 success
 *            - 1 soft reset failed
 *            - 3 handle is not initialized
 * @note      none
 */
uint8_t sht31_soft_reset()
{
    uint8_t res;

    if (sht31_handle.inited != 1) /* check handle initialization */
    {
        return 3; /* return error */
    }

    res = a_sht31_write(SHT31_COMMAND_SOFT_RESET); /* write command */
    if (res != 0) /* check result */
    {
        debug_print("sht31: write soft reset command failed.\n"); /* write command failed */
        return 1; /* return error */
    }

    return 0; /* success return 0 */
}

/**
 * @brief     启动或停止 芯片加热器
 * @param[in] enable is a bool value
 * @return    status code
 *            - 0 success
 *            - 1 set heater failed
 *            - 3 handle is not initialized
 * @note      none
 */
uint8_t sht31_set_heater(sht31_bool_t enable)
{
    uint8_t res;
    uint16_t command;

    if (sht31_handle.inited != 1) /* check handle initialization */
    {
        return 3; /* return error */
    }

    if (enable == SHT31_BOOL_TRUE) {
        /* enable heater */
        command = SHT31_COMMAND_HEATER_ENABLE; /* set enable */

    } else if (enable == SHT31_BOOL_FALSE) {
        /* disable heater */
        command = SHT31_COMMAND_HEATER_DISABLE; /* set disable */

    } else {
        debug_print("sht31: bool is invalid.\n"); /* bool is invalid */
        return 1; /* return error */
    }

    res = a_sht31_write(command); /* write command */
    if (res != 0) /* check result */
    {
        debug_print("sht31: write 0x%x command failed.\n", command); /* write command failed */
        return 1; /* return error */
    }

    return 0; /* success return 0 */
}
