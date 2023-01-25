#ifndef DRIVER_SHT31_H
#define DRIVER_SHT31_H

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C"{
#endif

/**
 * @defgroup sht31_driver sht31 driver function
 * @brief    sht31 driver modules
 * @{
 */

/**
 * @addtogroup sht31_base_driver
 * @{
 */

/**
 * @brief sht31 address enumeration definition
 */
typedef enum
{
    SHT31_ADDRESS_0 = (0x44 << 1),        /**< ADDR pin connected to GND */
    SHT31_ADDRESS_1 = (0x45 << 1),        /**< ADDR pin connected to VCC */
} sht31_address_t;

/**
 * @brief sht31 bool enumeration definition
 */
typedef enum
{
    SHT31_BOOL_FALSE = 0x00,        /**< disable function */
    SHT31_BOOL_TRUE  = 0x01,        /**< enable function */
} sht31_bool_t;

/**
 * @brief sht31 rate enumeration definition
 */
typedef enum
{
    SHT31_RATE_0P5HZ = 0x20,        /**< 0.5Hz sample rate */
    SHT31_RATE_1HZ   = 0x21,        /**< 1Hz sample rate */
    SHT31_RATE_2HZ   = 0x22,        /**< 2Hz sample rate */
    SHT31_RATE_4HZ   = 0x23,        /**< 4Hz sample rate */
    SHT31_RATE_10HZ  = 0x27,        /**< 10Hz sample rate */
} sht31_rate_t;

/**
 * @brief sht31 repeatability enumeration definition
 */
typedef enum  
{
    SHT31_REPEATABILITY_HIGH   = 0x00,        /**< high repeatability */
    SHT31_REPEATABILITY_MEDIUM = 0x01,        /**< medium repeatability */
    SHT31_REPEATABILITY_LOW    = 0x02,        /**< low repeatability */
} sht31_repeatability_t;

/**
 * @brief sht31 status enumeration definition
 */
typedef enum  
{
    SHT31_STATUS_ALERT_PENDING_STATUS = (1 << 15),        /**< alert pending status */
    SHT31_STATUS_HEATER_ON            = (1 << 13),        /**< heater on */
    SHT31_STATUS_HUMIDITY_ALERT       = (1 << 11),        /**< humidity alert */
    SHT31_STATUS_TEMPERATURE_ALERT    = (1 << 10),        /**< temperature alert */
    SHT31_STATUS_SYSTEM_RESET         = (1 << 4),         /**< system reset */
    SHT31_STATUS_COMMAND_STATUS       = (1 << 1),         /**< command status */
    SHT31_STATUS_CHECKSUM_STATUS      = (1 << 0),         /**< checksum status */
} sht31_status_t;

/**
 * @brief sht31 handle structure definition
 */
typedef struct sht31_handle_s
{
    uint8_t repeatability;                                                                         /**< repeatability value */
    uint8_t inited;                                                                                /**< inited flag */
} sht31_handle_t;

/**
 * @brief sht31 information structure definition
 */
typedef struct sht31_info_s
{
    char chip_name[32];                /**< chip name */
    char manufacturer_name[32];        /**< manufacturer name */
    char interface[8];                 /**< chip interface name */
    float supply_voltage_min_v;        /**< chip min supply voltage */
    float supply_voltage_max_v;        /**< chip max supply voltage */
    float max_current_ma;              /**< chip max current */
    float temperature_min;             /**< chip min operating temperature */
    float temperature_max;             /**< chip max operating temperature */
    uint32_t driver_version;           /**< driver version */
} sht31_info_t;


/**
 * @defgroup sht31_base_driver sht31 base driver function
 * @brief    sht31 base driver modules
 * @ingroup  sht31_driver
 * @{
 */


/**
 * @brief     initialize the chip
 * @return    status code
 *            - 0 success
 *            - 1 iic initialization failed
 *            - 2 handle is NULL
 *            - 3 linked functions is NULL
 * @note      none
 */
uint8_t sht31_init();

/**
 * @brief     close the chip
 * @return    status code
 *            - 0 success
 *            - 1 iic deinit failed
 *            - 2 handle is NULL
 *            - 3 handle is not initialized
 * @note      none
 */
uint8_t sht31_deinit();

uint8_t sht31_single_command(sht31_bool_t clock_stretching_enable);

/**
 * @brief      read data once
 * @param[in]  clock_stretching_enable is a clock stretching bool value
 * @param[out] *temperature_raw points to a raw temperature buffer
 * @param[out] *temperature_s points to a converted temperature buffer
 * @param[out] *humidity_raw points to a raw humidity buffer
 * @param[out] *humidity_s points to a converted humidity buffer
 * @return     status code
 *             - 0 success
 *             - 1 single read failed
 *             - 2 handle is NULL
 *             - 3 handle is not initialized
 * @note       none
 */
uint8_t sht31_single_read(uint16_t *temperature_raw, float *temperature_s, uint16_t *humidity_raw, float *humidity_s);

/**
 * @brief     start reading
 * @param[in] rate is the sample rate
 * @return    status code
 *            - 0 success
 *            - 1 start continuous read failed
 *            - 2 handle is NULL
 *            - 3 handle is not initialized
 * @note      none
 */
uint8_t sht31_start_continuous_read(sht31_rate_t rate);

/**
 * @brief     stop reading
 * @return    status code
 *            - 0 success
 *            - 1 stop continuous read failed
 *            - 2 handle is NULL
 *            - 3 handle is not initialized
 * @note      none
 */
uint8_t sht31_stop_continuous_read();

/**
 * @brief      read data continuously
 * @param[out] *temperature_raw points to a raw temperature buffer
 * @param[out] *temperature_s points to a converted temperature buffer
 * @param[out] *humidity_raw points to a raw humidity buffer
 * @param[out] *humidity_s points to a converted humidity buffer
 * @return     status code
 *             - 0 success
 *             - 1 continuous read failed
 *             - 2 handle is NULL
 *             - 3 handle is not initialized
 * @note       none
 */
uint8_t sht31_continuous_read(uint16_t *temperature_raw, float *temperature_s, uint16_t *humidity_raw, float *humidity_s);

/**
 * @brief      get the current status
 * @param[out] *status points to a status buffer
 * @return      status code
 *              - 0 success
 *              - 1 get status failed
 *              - 2 handle is NULL
 * @note        none
 */
uint8_t sht31_get_status(uint16_t *status);

/**
 * @brief     clear the current status
 * @return    status code
 *            - 0 success
 *            - 1 clear status failed
 *            - 2 handle is NULL
 * @note      none
 */
uint8_t sht31_clear_status();

/**
 * @brief     set the measurement repeatability
 * @param[in] repeatability is the measurement repeatability
 * @return    status code
 *            - 0 success
 *            - 1 set repeatability failed
 *            - 2 handle is NULL
 * @note      none
 */
uint8_t sht31_set_repeatability(sht31_repeatability_t repeatability);

/**
 * @brief      get the measurement repeatability
 * @param[out] *repeatability points to a measurement repeatability buffer
 * @return     status code
 *             - 0 success
 *             - 1 get repeatability failed
 *             - 2 handle is NULL
 * @note       none
 */
uint8_t sht31_get_repeatability(sht31_repeatability_t *repeatability);

/**
 * @brief     set the chip art
 * @return    status code
 *            - 0 success
 *            - 1 set art failed
 *            - 2 handle is NULL
 *            - 3 handle is not initialized
 * @note      none
 */
uint8_t sht31_set_art();

/**
 * @brief     soft reset the chip
 * @return    status code
 *            - 0 success
 *            - 1 soft reset failed
 *            - 2 handle is NULL
 *            - 3 handle is not initialized
 * @note      none
 */
uint8_t sht31_soft_reset();

/**
 * @brief     enable or disable the chip heater
 * @param[in] enable is a bool value
 * @return    status code
 *            - 0 success
 *            - 1 set heater failed
 *            - 2 handle is NULL
 *            - 3 handle is not initialized
 * @note      none
 */
uint8_t sht31_set_heater(sht31_bool_t enable);


#ifdef __cplusplus
}
#endif

#endif
