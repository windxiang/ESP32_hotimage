#include "settings.h"
#include "dispcolor.h"
#include "nvs.h"
#include "nvs_flash.h"
// #include "ui.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <string.h>

static nvs_handle SettingsHandle;

structSettingsParms settingsParms = {
    Emissivity : 0.95,
    ScaleMode : LINEAR,
    MLX90640FPS : 4,
    Resolution : 2,
    AutoScaleMode : 1,
    minTempNew : SCALE_DEFAULT_MIN,
    maxTempNew : SCALE_DEFAULT_MAX,
    TempMarkers : 1,
    ColorScale : Iron,
    LcdBrightness : 50,
    FuncUp : Markers_OnOff,
    FuncCenter : Save_BMP16,
    FuncDown : Scale_Prev,
};

// 设置存储 初始化
int settings_storage_init(void)
{
    esp_err_t err;

    // 初始化 NVS
    err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS分区被截断，需要擦除
        err = nvs_flash_erase();
        if (err != ESP_OK)
            return err;

        // 重试 nvs_flash_init
        err = nvs_flash_init();
        if (err != ESP_OK)
            return err;
		
		settings_write_all();
    }
    return err;
}

/**
 * @brief 保存
 *
 * @return int32_t
 */
int32_t settings_commit(void)
{
    return nvs_commit(SettingsHandle);
}

/**
 * @brief 设置读取
 *
 * @param pKey
 * @param type
 * @param pValue
 * @return int
 */
int setting_read(char* pKey, eType type, void* pValue)
{
    size_t len;

    switch (type) {
    case int8:
        return nvs_get_i8(SettingsHandle, pKey, (int8_t*)pValue);

    case uint8:
        return nvs_get_u8(SettingsHandle, pKey, (uint8_t*)pValue);

    case int16:
        return nvs_get_i16(SettingsHandle, pKey, (int16_t*)pValue);

    case uint16:
        return nvs_get_u16(SettingsHandle, pKey, (uint16_t*)pValue);

    case int32:
        return nvs_get_i32(SettingsHandle, pKey, (int32_t*)pValue);

    case float32:
    case uint32:
        return nvs_get_u32(SettingsHandle, pKey, (uint32_t*)pValue);

    case int64:
        return nvs_get_i64(SettingsHandle, pKey, (int64_t*)pValue);

    case uint64:
        return nvs_get_u64(SettingsHandle, pKey, (uint64_t*)pValue);

    case str:
        return nvs_get_str(SettingsHandle, pKey, (char*)pValue, &len);
    }

    return 0;
}

/**
 * @brief 设置保存
 *
 * @param pKey
 * @param type
 * @param pValue
 * @return int
 */
int setting_write(char* pKey, eType type, void* pValue)
{
    esp_err_t err = ESP_OK;

    switch (type) {
    case int8:
        err = nvs_set_i8(SettingsHandle, pKey, *((int8_t*)pValue));
        break;

    case uint8:
        err = nvs_set_u8(SettingsHandle, pKey, *((uint8_t*)pValue));
        break;

    case int16:
        err = nvs_set_i16(SettingsHandle, pKey, *((int16_t*)pValue));
        break;

    case uint16:
        err = nvs_set_u16(SettingsHandle, pKey, *((uint16_t*)pValue));
        break;

    case int32:
        err = nvs_set_i32(SettingsHandle, pKey, *((int32_t*)pValue));
        break;

    case float32:
    case uint32:
        err = nvs_set_u32(SettingsHandle, pKey, *((uint32_t*)pValue));
        break;

    case int64:
        err = nvs_set_i64(SettingsHandle, pKey, *((int64_t*)pValue));
        break;

    case uint64:
        err = nvs_set_u64(SettingsHandle, pKey, *((uint64_t*)pValue));
        break;

    case str:
        err = nvs_set_str(SettingsHandle, pKey, ((char*)pValue));
        break;
    }

    if (err != ESP_OK)
        return err;

    return 0;
}

// 读取所有配置
int settings_read_all(void)
{
    esp_err_t err = ESP_OK;

    if (0 == SettingsHandle)
    {
        if (nvs_open("settings", NVS_READWRITE, &SettingsHandle) != ESP_OK)
            return -1;
    }

    err = setting_read("Emissivity", float32, &settingsParms.Emissivity);
    if (err != ESP_OK)
        return err;

    err = setting_read("ScaleMode", uint8, &settingsParms.ScaleMode);
    if (err != ESP_OK)
        return err;

    err = setting_read("MLX90640FPS", uint8, &settingsParms.MLX90640FPS);
    if (err != ESP_OK)
        return err;

    err = setting_read("MLX90640Res", uint8, &settingsParms.Resolution);
    if (err != ESP_OK)
        return err;

    err = setting_read("AutoScaleMode", uint8, &settingsParms.AutoScaleMode);
    if (err != ESP_OK)
        return err;

    err = setting_read("minTempNew", float32, &settingsParms.minTempNew);
    if (err != ESP_OK)
        return err;

    err = setting_read("maxTempNew", float32, &settingsParms.maxTempNew);
    if (err != ESP_OK)
        return err;

    err = setting_read("TempMarkers", uint8, &settingsParms.TempMarkers);
    if (err != ESP_OK)
        return err;

    err = setting_read("ColorScale", uint8, &settingsParms.ColorScale);
    if (err != ESP_OK)
        return err;

    err = setting_read("LcdBrightness", int32, &settingsParms.LcdBrightness);
    if (err != ESP_OK)
        return err;

    err = setting_read("FuncUp", uint8, &settingsParms.FuncUp);
    if (err != ESP_OK)
        return err;

    err = setting_read("FuncCenter", uint8, &settingsParms.FuncCenter);
    if (err != ESP_OK)
        return err;

    err = setting_read("FuncDown", uint8, &settingsParms.FuncDown);
    if (err != ESP_OK)
        return err;

    return 0;
}

// 写入所有配置
int settings_write_all(void)
{
    esp_err_t err = ESP_OK;

    if (0 == SettingsHandle)
    {
        if (nvs_open("settings", NVS_READWRITE, &SettingsHandle) != ESP_OK)
            return -1;
    }

    err = setting_write("Emissivity", float32, &settingsParms.Emissivity);
    if (err != ESP_OK)
        return err;

    err = setting_write("ScaleMode", uint8, &settingsParms.ScaleMode);
    if (err != ESP_OK)
        return err;

    err = setting_write("MLX90640FPS", uint8, &settingsParms.MLX90640FPS);
    if (err != ESP_OK)
        return err;

    err = setting_write("MLX90640Res", uint8, &settingsParms.Resolution);
    if (err != ESP_OK)
        return err;

    err = setting_write("AutoScaleMode", uint8, &settingsParms.AutoScaleMode);
    if (err != ESP_OK)
        return err;

    err = setting_write("minTempNew", float32, &settingsParms.minTempNew);
    if (err != ESP_OK)
        return err;

    err = setting_write("maxTempNew", float32, &settingsParms.maxTempNew);
    if (err != ESP_OK)
        return err;

    err = setting_write("TempMarkers", uint8, &settingsParms.TempMarkers);
    if (err != ESP_OK)
        return err;

    err = setting_write("ColorScale", uint8, &settingsParms.ColorScale);
    if (err != ESP_OK)
        return err;

    err = setting_write("LcdBrightness", int32, &settingsParms.LcdBrightness);
    if (err != ESP_OK)
        return err;

    err = setting_write("FuncUp", uint8, &settingsParms.FuncUp);
    if (err != ESP_OK)
        return err;

    err = setting_write("FuncCenter", uint8, &settingsParms.FuncCenter);
    if (err != ESP_OK)
        return err;

    err = setting_write("FuncDown", uint8, &settingsParms.FuncDown);
    if (err != ESP_OK)
        return err;

    return settings_commit();
}
