/**
 * @copyright (C) 2017 Melexis N.V.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */
#include "iic.h"
#include <MLX90640_I2C_Driver.h>
#include <driver_MLX90640.h>
#include <math.h>

void ExtractVDDParameters(uint16_t* eeData, paramsMLX90640* mlx90640);
void ExtractPTATParameters(uint16_t* eeData, paramsMLX90640* mlx90640);
void ExtractGainParameters(uint16_t* eeData, paramsMLX90640* mlx90640);
void ExtractTgcParameters(uint16_t* eeData, paramsMLX90640* mlx90640);
void ExtractResolutionParameters(uint16_t* eeData, paramsMLX90640* mlx90640);
void ExtractKsTaParameters(uint16_t* eeData, paramsMLX90640* mlx90640);
void ExtractKsToParameters(uint16_t* eeData, paramsMLX90640* mlx90640);
void ExtractAlphaParameters(uint16_t* eeData, paramsMLX90640* mlx90640);
void ExtractOffsetParameters(uint16_t* eeData, paramsMLX90640* mlx90640);
void ExtractKtaPixelParameters(uint16_t* eeData, paramsMLX90640* mlx90640);
void ExtractKvPixelParameters(uint16_t* eeData, paramsMLX90640* mlx90640);
void ExtractCPParameters(uint16_t* eeData, paramsMLX90640* mlx90640);
void ExtractCILCParameters(uint16_t* eeData, paramsMLX90640* mlx90640);
int ExtractDeviatingPixels(uint16_t* eeData, paramsMLX90640* mlx90640);
int CheckAdjacentPixels(uint16_t pix1, uint16_t pix2);
float GetMedian(float* values, int n);
int IsPixelBad(uint16_t pixel, paramsMLX90640* params);
int ValidateFrameData(uint16_t* frameData);
int ValidateAuxData(uint16_t* auxData);

/**
 * @brief 初始化IIC
 *
 */
void MLX90640_Init()
{
}

/**
 * @brief 获取 MLX90640 的EEPROM 大小
 *        MLX90640_DumpEE 函数会用到
 * @return uint16_t
 */
uint16_t MLX90640_getEEPROMSize()
{
    return 832;
}

/**
 * @brief 获取 MLX90640 的 帧缓存大小
 *         MLX90640_GetFrameData 函数会用到
 * @return uint16_t
 */
uint16_t MLX90640_getFrameSize()
{
    return 834;
}

/**
 * @brief 读取全部校正参数 读取整个EEPROM 缓存至少要832字节
 *
 * @param slaveAddr
 * @param eeData
 * @return int
 */
int MLX90640_DumpEE(uint8_t slaveAddr, uint16_t* eeData)
{
    return MLX90640_I2CRead(slaveAddr, 0x2400, MLX90640_getEEPROMSize(), eeData);
}

/**
 * @brief 等待新数据可用
 *
 * @param slaveAddr
 * @return int
 */
int MLX90640_SynchFrame(uint8_t slaveAddr)
{
    uint16_t dataReady = 0;
    uint16_t statusRegister;
    int error = 1;

    error = MLX90640_I2CWrite(slaveAddr, 0x8000, 0x0030);
    if (error == -1) {
        return error;
    }

    while (dataReady == 0) {
        error = MLX90640_I2CRead(slaveAddr, 0x8000, 1, &statusRegister);
        if (error != 0) {
            return error;
        }
        dataReady = statusRegister & 0x0008;
    }

    return 0;
}

/**
 * @brief 使用IIC中的全局复位命令
 *
 * @param slaveAddr
 * @return int
 */
int MLX90640_TriggerMeasurement(uint8_t slaveAddr)
{
    int error = 1;
    uint16_t ctrlReg;

    // 这个寄存器最高位 未在文档中说明
    error = MLX90640_I2CRead(slaveAddr, 0x800D, 1, &ctrlReg);
    if (error != 0) {
        return error;
    }

    ctrlReg |= 0x8000;
    error = MLX90640_I2CWrite(slaveAddr, 0x800D, ctrlReg);
    if (error != 0) {
        return error;
    }

    // 全局IIC设备复位
    error = i2c_general_reset();
    if (error != 0) {
        return error;
    }

    error = MLX90640_I2CRead(slaveAddr, 0x800D, 1, &ctrlReg);
    if (error != 0) {
        return error;
    }

    if ((ctrlReg & 0x8000) != 0) {
        return -9;
    }

    return 0;
}

/**
 * @brief 读取一帧实时数据 计算所需要的完整的一帧数据为 834 个字（包括 832个字 RAM 数据+控制寄存器+状态寄存器）
 *
 * @param slaveAddr
 * @param frameData
 * @return int 返回-1 表示 MLX90640 未应答， -8 表示读取异常（最可能的情况是读取速率太低了）
 *             返回 0 或者 1 则表示读取到了刚刚测量完成的子页 0 或者子页 1（读取成功）
 */
int MLX90640_GetFrameData(uint8_t slaveAddr, uint16_t* frameData)
{
    uint16_t dataReady = 0;
    uint16_t controlRegister1;
    uint16_t statusRegister;
    int error = 1;
    uint16_t data[64];
    uint8_t cnt = 0;

    // 读取寄存器 判断新数据是否准备好
    while (dataReady == 0) {
        error = MLX90640_I2CRead(slaveAddr, 0x8000, 1, &statusRegister);
        if (error != 0) {
            return error;
        }
        dataReady = statusRegister & 0x0008;
    }

    error = MLX90640_I2CWrite(slaveAddr, 0x8000, 0x0030);
    if (error == -1) {
        return error;
    }

    // 读取帧缓存
    error = MLX90640_I2CRead(slaveAddr, 0x0400, 768, frameData);
    if (error != 0) {
        return error;
    }

    error = MLX90640_I2CRead(slaveAddr, 0x0700, 64, data);
    if (error != 0) {
        return error;
    }

    error = MLX90640_I2CRead(slaveAddr, 0x800D, 1, &controlRegister1);
    frameData[832] = controlRegister1;
    frameData[833] = statusRegister & 0x0001; // 新的一帧 在page0还是page1

    if (error != 0) {
        return error;
    }

    error = ValidateAuxData(data);
    if (error == 0) {
        for (cnt = 0; cnt < 64; cnt++) {
            frameData[cnt + 768] = data[cnt];
        }
    }

    error = ValidateFrameData(frameData);
    if (error != 0) {
        return error;
    }

    return frameData[833];
}

/**
 * @brief 判断有效的帧数据
 *
 * @param frameData
 * @return int
 */
int ValidateFrameData(uint16_t* frameData)
{
    uint8_t line = 0;

    for (int i = 0; i < 768; i += 32) {
        if ((frameData[i] == 0x7FFF) && (line % 2 == frameData[833]))
            return -8;
        line = line + 1;
    }

    return 0;
}

int ValidateAuxData(uint16_t* auxData)
{
    if (auxData[0] == 0x7FFF)
        return -8;

    for (int i = 8; i < 19; i++) {
        if (auxData[i] == 0x7FFF)
            return -8;
    }

    for (int i = 20; i < 23; i++) {
        if (auxData[i] == 0x7FFF)
            return -8;
    }

    for (int i = 24; i < 33; i++) {
        if (auxData[i] == 0x7FFF)
            return -8;
    }

    for (int i = 40; i < 51; i++) {
        if (auxData[i] == 0x7FFF)
            return -8;
    }

    for (int i = 52; i < 55; i++) {
        if (auxData[i] == 0x7FFF)
            return -8;
    }

    for (int i = 56; i < 64; i++) {
        if (auxData[i] == 0x7FFF)
            return -8;
    }

    return 0;
}

/**
 * @brief 解析MLX90640_DumpEE函数读取的数据为计算参数
 *
 * @param eeData
 * @param mlx90640
 * @return int 返回-7 表示提供的 EEPROM 参数错误
 */
int MLX90640_ExtractParameters(uint16_t* eeData, paramsMLX90640* mlx90640)
{
    int error = 0;

    ExtractVDDParameters(eeData, mlx90640);
    ExtractPTATParameters(eeData, mlx90640);
    ExtractGainParameters(eeData, mlx90640);
    ExtractTgcParameters(eeData, mlx90640);
    ExtractResolutionParameters(eeData, mlx90640);
    ExtractKsTaParameters(eeData, mlx90640);
    ExtractKsToParameters(eeData, mlx90640);
    ExtractCPParameters(eeData, mlx90640);
    ExtractAlphaParameters(eeData, mlx90640);
    ExtractOffsetParameters(eeData, mlx90640);
    ExtractKtaPixelParameters(eeData, mlx90640);
    ExtractKvPixelParameters(eeData, mlx90640);
    ExtractCILCParameters(eeData, mlx90640);
    error = ExtractDeviatingPixels(eeData, mlx90640);

    return error;
}

//------------------------------------------------------------------------------

/**
 * @brief 此函数用于设置 MLX90640 的测量分辨率值
 *
 * @param slaveAddr
 * @param resolution 0~3 表示分辨率为 16~19 位
 * @return int  返回 0 表示设置成功， -1 表示设备未应答， -2 表示重新读取后发现不是预期的值。
 */
int MLX90640_SetResolution(uint8_t slaveAddr, uint8_t resolution)
{
    uint16_t controlRegister1;
    int value;
    int error;

    value = (resolution & 0x03) << 10;

    error = MLX90640_I2CRead(slaveAddr, 0x800D, 1, &controlRegister1);

    if (error == 0) {
        value = (controlRegister1 & 0xF3FF) | value;
        error = MLX90640_I2CWrite(slaveAddr, 0x800D, value);
    }

    return error;
}

/**
 * @brief 此函数用于读取当前的测量分辨率
 *
 * @param slaveAddr
 * @return int 若此函数返回了-1 则表示读取失败。  0~3标识当前分辨率为 16~19位
 */
int MLX90640_GetCurResolution(uint8_t slaveAddr)
{
    uint16_t controlRegister1;
    int resolutionRAM;
    int error;

    error = MLX90640_I2CRead(slaveAddr, 0x800D, 1, &controlRegister1);
    if (error != 0) {
        return error;
    }
    resolutionRAM = (controlRegister1 & 0x0C00) >> 10;

    return resolutionRAM;
}

//------------------------------------------------------------------------------

/**
 * @brief 此函数用于设置 MLX90640 的测量速率（即：每秒测量几帧数据）
 * 上电复位后,会恢复EEPROM中的帧率
 *
 * @param slaveAddr
 * @param refreshRate 帧率 参数值可以是 0~7 代表 0.5、1、 2、 4、 8、 16、 32 和 64Hz
 * @return int 返回 0 表示设置成功， -1 表示设备未应答， -2 表示重新读取后发现不是预期的值
 */
int MLX90640_SetRefreshRate(uint8_t slaveAddr, uint8_t refreshRate)
{
    uint16_t controlRegister1;
    int value;
    int error;

    value = (refreshRate & 0x07) << 7;

    error = MLX90640_I2CRead(slaveAddr, 0x800D, 1, &controlRegister1);
    if (error == 0) {
        value = (controlRegister1 & 0xFC7F) | value;
        error = MLX90640_I2CWrite(slaveAddr, 0x800D, value);
    }

    return error;
}

/**
 * @brief 此函数用于读取当前的测量速率值
 *
 * @param slaveAddr
 * @return int 此函数返回了-1 则表示读取失败, 返回0~7代表帧率 0.5、1、 2、 4、 8、 16、 32 和 64Hz
 */
int MLX90640_GetRefreshRate(uint8_t slaveAddr)
{
    uint16_t controlRegister1;
    int refreshRate;
    int error;

    error = MLX90640_I2CRead(slaveAddr, 0x800D, 1, &controlRegister1);
    if (error != 0) {
        return error;
    }
    refreshRate = (controlRegister1 & 0x0380) >> 7;

    return refreshRate;
}

//------------------------------------------------------------------------------

/**
 * @brief 设置为行交错模式 （TV模式）
 *
 * @param slaveAddr
 * @return int 返回 0 表示设置成功， -1 表示设备未应答， -2 表示重新读取后发现不是预期的值。
 */
int MLX90640_SetInterleavedMode(uint8_t slaveAddr)
{
    uint16_t controlRegister1;
    int value;
    int error;

    error = MLX90640_I2CRead(slaveAddr, 0x800D, 1, &controlRegister1);

    if (error == 0) {
        value = (controlRegister1 & 0xEFFF);
        error = MLX90640_I2CWrite(slaveAddr, 0x800D, value);
    }

    return error;
}

/**
 * @brief 设置成棋盘模式（像素交错模式）
 *
 * @param slaveAddr
 * @return int 返回 0 表示设置成功， -1 表示设备未应答， -2 表示重新读取后发现不是预期的值。
 */
int MLX90640_SetChessMode(uint8_t slaveAddr)
{
    uint16_t controlRegister1;
    int value;
    int error;

    error = MLX90640_I2CRead(slaveAddr, 0x800D, 1, &controlRegister1);

    if (error == 0) {
        value = (controlRegister1 | 0x1000);
        error = MLX90640_I2CWrite(slaveAddr, 0x800D, value);
    }

    return error;
}

/**
 * @brief 读取当前的测量模式
 *
 * @param slaveAddr
 * @return int 返回 0 表示工作于 TV 模式，返回 1 表示工作于棋盘模式。
 */
int MLX90640_GetCurMode(uint8_t slaveAddr)
{
    uint16_t controlRegister1;
    int modeRAM;
    int error;

    error = MLX90640_I2CRead(slaveAddr, 0x800D, 1, &controlRegister1);
    if (error != 0) {
        return error;
    }
    modeRAM = (controlRegister1 & 0x1000) >> 12;

    return modeRAM;
}

//------------------------------------------------------------------------------

/**
 * @brief 计算物体绝对温度数据(32*24=768像素)
 *
 * @param frameData 读取到的一帧实时数据
 * @param params 从EEPROM解析的数据
 * @param emissivity 被测物体的辐射率（人体为 0.95）
 * @param tr 校正温度，一般取 Ta-8
 * @param result 计算结果， 768个浮点数， 单位为℃温度值
 */
void MLX90640_CalculateTo(uint16_t* frameData, const paramsMLX90640* params, float emissivity, float tr, float* result)
{
    float vdd;
    float ta;
    float ta4;
    float tr4;
    float taTr;
    float gain;
    float irDataCP[2];
    float irData;
    float alphaCompensated;
    uint8_t mode;
    int8_t ilPattern;
    int8_t chessPattern;
    int8_t pattern;
    int8_t conversionPattern;
    float Sx;
    float To;
    float alphaCorrR[4];
    int8_t range;
    uint16_t subPage;
    float ktaScale;
    float kvScale;
    float alphaScale;
    float kta;
    float kv;

    subPage = frameData[833]; // 得到当前的子页
    vdd = MLX90640_GetVdd(frameData, params);
    ta = MLX90640_GetTa(frameData, params);

    ta4 = (ta + 273.15);
    ta4 = ta4 * ta4;
    ta4 = ta4 * ta4;
    tr4 = (tr + 273.15);
    tr4 = tr4 * tr4;
    tr4 = tr4 * tr4;
    taTr = tr4 - (tr4 - ta4) / emissivity;

    ktaScale = pow(2, (double)params->ktaScale);
    kvScale = pow(2, (double)params->kvScale);
    alphaScale = pow(2, (double)params->alphaScale);

    alphaCorrR[0] = 1 / (1 + params->ksTo[0] * 40);
    alphaCorrR[1] = 1;
    alphaCorrR[2] = (1 + params->ksTo[1] * params->ct[2]);
    alphaCorrR[3] = alphaCorrR[2] * (1 + params->ksTo[2] * (params->ct[3] - params->ct[2]));

    //------------------------- Gain calculation -----------------------------------
    gain = frameData[778];
    if (gain > 32767) {
        gain = gain - 65536;
    }

    gain = params->gainEE / gain;

    //------------------------- To calculation -------------------------------------
    mode = (frameData[832] & 0x1000) >> 5;

    irDataCP[0] = frameData[776];
    irDataCP[1] = frameData[808];
    for (int i = 0; i < 2; i++) {
        if (irDataCP[i] > 32767) {
            irDataCP[i] = irDataCP[i] - 65536;
        }
        irDataCP[i] = irDataCP[i] * gain;
    }
    irDataCP[0] = irDataCP[0] - params->cpOffset[0] * (1 + params->cpKta * (ta - 25)) * (1 + params->cpKv * (vdd - 3.3));
    if (mode == params->calibrationModeEE) {
        irDataCP[1] = irDataCP[1] - params->cpOffset[1] * (1 + params->cpKta * (ta - 25)) * (1 + params->cpKv * (vdd - 3.3));
    } else {
        irDataCP[1] = irDataCP[1] - (params->cpOffset[1] + params->ilChessC[0]) * (1 + params->cpKta * (ta - 25)) * (1 + params->cpKv * (vdd - 3.3));
    }

    for (int pixelNumber = 0; pixelNumber < 768; pixelNumber++) {
        ilPattern = pixelNumber / 32 - (pixelNumber / 64) * 2;
        chessPattern = ilPattern ^ (pixelNumber - (pixelNumber / 2) * 2);
        conversionPattern = ((pixelNumber + 2) / 4 - (pixelNumber + 3) / 4 + (pixelNumber + 1) / 4 - pixelNumber / 4) * (1 - 2 * ilPattern);

        if (mode == 0) {
            pattern = ilPattern;
        } else {
            pattern = chessPattern;
        }

        if (pattern == frameData[833]) {
            irData = frameData[pixelNumber];
            if (irData > 32767) {
                irData = irData - 65536;
            }
            irData = irData * gain;

            kta = params->kta[pixelNumber] / ktaScale;
            kv = params->kv[pixelNumber] / kvScale;
            irData = irData - params->offset[pixelNumber] * (1 + kta * (ta - 25)) * (1 + kv * (vdd - 3.3));

            if (mode != params->calibrationModeEE) {
                irData = irData + params->ilChessC[2] * (2 * ilPattern - 1) - params->ilChessC[1] * conversionPattern;
            }

            irData = irData - params->tgc * irDataCP[subPage];
            irData = irData / emissivity;

            alphaCompensated = SCALEALPHA * alphaScale / params->alpha[pixelNumber];
            alphaCompensated = alphaCompensated * (1 + params->KsTa * (ta - 25));

            Sx = alphaCompensated * alphaCompensated * alphaCompensated * (irData + alphaCompensated * taTr);
            Sx = sqrt(sqrt(Sx)) * params->ksTo[1];

            To = sqrt(sqrt(irData / (alphaCompensated * (1 - params->ksTo[1] * 273.15) + Sx) + taTr)) - 273.15;

            if (To < params->ct[1]) {
                range = 0;
            } else if (To < params->ct[2]) {
                range = 1;
            } else if (To < params->ct[3]) {
                range = 2;
            } else {
                range = 3;
            }

            To = sqrt(sqrt(irData / (alphaCompensated * alphaCorrR[range] * (1 + params->ksTo[range] * (To - params->ct[range]))) + taTr)) - 273.15;

            result[pixelNumber] = To;
        }
    }
}

//------------------------------------------------------------------------------

/**
 * @brief 此函数的功能与MLX90640_CalculateTo计算温度几乎完全相同，不同点仅为计算结果中的数值没有规划为温度单位，而是一些仅有数值大小意义的数值，用这些数值大小来绘图是足够的，
 *        这个函数的优点就是速度要比计算温度要快很多（仅需要绘图而不关心绝对温度值时可以使用这个函数来计算完成）。
 *
 * @param frameData
 * @param params
 * @param result
 */
void MLX90640_GetImage(uint16_t* frameData, const paramsMLX90640* params, float* result)
{
    float vdd;
    float ta;
    float gain;
    float irDataCP[2];
    float irData;
    float alphaCompensated;
    uint8_t mode;
    int8_t ilPattern;
    int8_t chessPattern;
    int8_t pattern;
    int8_t conversionPattern;
    float image;
    uint16_t subPage;
    float ktaScale;
    float kvScale;
    float kta;
    float kv;

    subPage = frameData[833]; // 得到当前的子页
    vdd = MLX90640_GetVdd(frameData, params);
    ta = MLX90640_GetTa(frameData, params);

    ktaScale = pow(2, (double)params->ktaScale);
    kvScale = pow(2, (double)params->kvScale);

    //------------------------- Gain calculation -----------------------------------
    gain = frameData[778];
    if (gain > 32767) {
        gain = gain - 65536;
    }

    gain = params->gainEE / gain;

    //------------------------- Image calculation -------------------------------------
    mode = (frameData[832] & 0x1000) >> 5;

    irDataCP[0] = frameData[776];
    irDataCP[1] = frameData[808];
    for (int i = 0; i < 2; i++) {
        if (irDataCP[i] > 32767) {
            irDataCP[i] = irDataCP[i] - 65536;
        }
        irDataCP[i] = irDataCP[i] * gain;
    }
    irDataCP[0] = irDataCP[0] - params->cpOffset[0] * (1 + params->cpKta * (ta - 25)) * (1 + params->cpKv * (vdd - 3.3));
    if (mode == params->calibrationModeEE) {
        irDataCP[1] = irDataCP[1] - params->cpOffset[1] * (1 + params->cpKta * (ta - 25)) * (1 + params->cpKv * (vdd - 3.3));
    } else {
        irDataCP[1] = irDataCP[1] - (params->cpOffset[1] + params->ilChessC[0]) * (1 + params->cpKta * (ta - 25)) * (1 + params->cpKv * (vdd - 3.3));
    }

    for (int pixelNumber = 0; pixelNumber < 768; pixelNumber++) {
        ilPattern = pixelNumber / 32 - (pixelNumber / 64) * 2;
        chessPattern = ilPattern ^ (pixelNumber - (pixelNumber / 2) * 2);
        conversionPattern = ((pixelNumber + 2) / 4 - (pixelNumber + 3) / 4 + (pixelNumber + 1) / 4 - pixelNumber / 4) * (1 - 2 * ilPattern);

        if (mode == 0) {
            pattern = ilPattern;
        } else {
            pattern = chessPattern;
        }

        if (pattern == frameData[833]) {
            irData = frameData[pixelNumber];
            if (irData > 32767) {
                irData = irData - 65536;
            }
            irData = irData * gain;

            kta = params->kta[pixelNumber] / ktaScale;
            kv = params->kv[pixelNumber] / kvScale;
            irData = irData - params->offset[pixelNumber] * (1 + kta * (ta - 25)) * (1 + kv * (vdd - 3.3));

            if (mode != params->calibrationModeEE) {
                irData = irData + params->ilChessC[2] * (2 * ilPattern - 1) - params->ilChessC[1] * conversionPattern;
            }

            irData = irData - params->tgc * irDataCP[subPage];

            alphaCompensated = params->alpha[pixelNumber];

            image = irData * alphaCompensated;

            result[pixelNumber] = image;
        }
    }
}

//------------------------------------------------------------------------------

/**
 * @brief 计算并返回 Vdd 电压值
 *
 * @param frameData
 * @param params
 * @return float 返回值是浮点数，单位为 V。若不是 3.3V 左右，则说明发生了较为严重的问题
 */
float MLX90640_GetVdd(uint16_t* frameData, const paramsMLX90640* params)
{
    float vdd;
    float resolutionCorrection;

    int resolutionRAM;

    vdd = frameData[810];
    if (vdd > 32767) {
        vdd = vdd - 65536;
    }
    resolutionRAM = (frameData[832] & 0x0C00) >> 10;
    resolutionCorrection = pow(2, (double)params->resolutionEE) / pow(2, (double)resolutionRAM);
    vdd = (resolutionCorrection * vdd - params->vdd25) / params->kVdd + 3.3;

    return vdd;
}

//------------------------------------------------------------------------------

/**
 * @brief 计算得到 Ta（ MLX90640 外壳温度）
 *
 * @param frameData
 * @param params
 * @return float 返回值是浮点数，单位为℃。若与环境温度相差甚远，则说明发生了较为严重的问题
 */
float MLX90640_GetTa(uint16_t* frameData, const paramsMLX90640* params)
{
    float ptat;
    float ptatArt;
    float vdd;
    float ta;

    vdd = MLX90640_GetVdd(frameData, params);

    ptat = frameData[800];
    if (ptat > 32767) {
        ptat = ptat - 65536;
    }

    ptatArt = frameData[768];
    if (ptatArt > 32767) {
        ptatArt = ptatArt - 65536;
    }
    ptatArt = (ptat / (ptat * params->alphaPTAT + ptatArt)) * pow(2, (double)18);

    ta = (ptatArt / (1 + params->KvPTAT * (vdd - 3.3)) - params->vPTAT25);
    ta = ta / params->KtPTAT + 25;

    return ta;
}

//------------------------------------------------------------------------------

/**
 * @brief 返回值表示当前帧是哪个子页
 *
 * @param frameData
 * @return int 返回值表示当前帧是哪个子页（ 0 或 1 ）
 */
int MLX90640_GetSubPageNumber(uint16_t* frameData)
{
    return frameData[833];
}

//------------------------------------------------------------------------------
/**
 * @brief 校正损坏像素 或 异常像素的值
 *
 * @param pixels 要校正的像素的数组的指针
 * @param to
 * @param mode 0 交错模式  1 棋盘模式
 * @param params
 */
void MLX90640_BadPixelsCorrection(uint16_t* pixels, float* to, int mode, paramsMLX90640* params)
{
    float ap[4];
    uint8_t pix;
    uint8_t line;
    uint8_t column;

    pix = 0;
    while (pixels[pix] != 0xFFFF) {
        line = pixels[pix] >> 5;
        column = pixels[pix] - (line << 5);

        if (mode == 1) {
            if (line == 0) {
                if (column == 0) {
                    to[pixels[pix]] = to[33];
                } else if (column == 31) {
                    to[pixels[pix]] = to[62];
                } else {
                    to[pixels[pix]] = (to[pixels[pix] + 31] + to[pixels[pix] + 33]) / 2.0;
                }
            } else if (line == 23) {
                if (column == 0) {
                    to[pixels[pix]] = to[705];
                } else if (column == 31) {
                    to[pixels[pix]] = to[734];
                } else {
                    to[pixels[pix]] = (to[pixels[pix] - 33] + to[pixels[pix] - 31]) / 2.0;
                }
            } else if (column == 0) {
                to[pixels[pix]] = (to[pixels[pix] - 31] + to[pixels[pix] + 33]) / 2.0;
            } else if (column == 31) {
                to[pixels[pix]] = (to[pixels[pix] - 33] + to[pixels[pix] + 31]) / 2.0;
            } else {
                ap[0] = to[pixels[pix] - 33];
                ap[1] = to[pixels[pix] - 31];
                ap[2] = to[pixels[pix] + 31];
                ap[3] = to[pixels[pix] + 33];
                to[pixels[pix]] = GetMedian(ap, 4);
            }
        } else {
            if (column == 0) {
                to[pixels[pix]] = to[pixels[pix] + 1];
            } else if (column == 1 || column == 30) {
                to[pixels[pix]] = (to[pixels[pix] - 1] + to[pixels[pix] + 1]) / 2.0;
            } else if (column == 31) {
                to[pixels[pix]] = to[pixels[pix] - 1];
            } else {
                if (IsPixelBad(pixels[pix] - 2, params) == 0 && IsPixelBad(pixels[pix] + 2, params) == 0) {
                    ap[0] = to[pixels[pix] + 1] - to[pixels[pix] + 2];
                    ap[1] = to[pixels[pix] - 1] - to[pixels[pix] - 2];
                    if (fabs(ap[0]) > fabs(ap[1])) {
                        to[pixels[pix]] = to[pixels[pix] - 1] + ap[1];
                    } else {
                        to[pixels[pix]] = to[pixels[pix] + 1] + ap[0];
                    }
                } else {
                    to[pixels[pix]] = (to[pixels[pix] - 1] + to[pixels[pix] + 1]) / 2.0;
                }
            }
        }
        pix = pix + 1;
    }
}

//------------------------------------------------------------------------------

/**
 * @brief 获得基准电压
 *
 * @param eeData
 * @param mlx90640
 */
void ExtractVDDParameters(uint16_t* eeData, paramsMLX90640* mlx90640)
{
    int16_t kVdd;
    int16_t vdd25;

    kVdd = eeData[51];

    // kVdd
    kVdd = (eeData[51] & 0xFF00) >> 8;
    if (kVdd > 127) {
        kVdd = kVdd - 256;
    }
    kVdd = 32 * kVdd;

    // vdd25
    vdd25 = eeData[51] & 0x00FF;
    vdd25 = ((vdd25 - 256) << 5) - 8192;

    mlx90640->kVdd = kVdd;
    mlx90640->vdd25 = vdd25;
}

//------------------------------------------------------------------------------

void ExtractPTATParameters(uint16_t* eeData, paramsMLX90640* mlx90640)
{
    float KvPTAT;
    float KtPTAT;
    int16_t vPTAT25;
    float alphaPTAT;

    KvPTAT = (eeData[50] & 0xFC00) >> 10;
    if (KvPTAT > 31) {
        KvPTAT = KvPTAT - 64;
    }
    KvPTAT = KvPTAT / 4096;

    KtPTAT = eeData[50] & 0x03FF;
    if (KtPTAT > 511) {
        KtPTAT = KtPTAT - 1024;
    }
    KtPTAT = KtPTAT / 8;

    vPTAT25 = eeData[49];

    alphaPTAT = (eeData[16] & 0xF000) / pow(2, (double)14) + 8.0f;

    mlx90640->KvPTAT = KvPTAT;
    mlx90640->KtPTAT = KtPTAT;
    mlx90640->vPTAT25 = vPTAT25;
    mlx90640->alphaPTAT = alphaPTAT;
}

//------------------------------------------------------------------------------

void ExtractGainParameters(uint16_t* eeData, paramsMLX90640* mlx90640)
{
    uint16_t gainEE;

    gainEE = eeData[48];
    if (gainEE > 32767) {
        gainEE = gainEE - 65536;
    }

    mlx90640->gainEE = gainEE;
}

//------------------------------------------------------------------------------

void ExtractTgcParameters(uint16_t* eeData, paramsMLX90640* mlx90640)
{
    float tgc;
    tgc = eeData[60] & 0x00FF;
    if (tgc > 127) {
        tgc = tgc - 256;
    }
    tgc = tgc / 32.0f;

    mlx90640->tgc = tgc;
}

//------------------------------------------------------------------------------

void ExtractResolutionParameters(uint16_t* eeData, paramsMLX90640* mlx90640)
{
    uint8_t resolutionEE;
    resolutionEE = (eeData[56] & 0x3000) >> 12;

    mlx90640->resolutionEE = resolutionEE;
}

//------------------------------------------------------------------------------

void ExtractKsTaParameters(uint16_t* eeData, paramsMLX90640* mlx90640)
{
    float KsTa;
    KsTa = (eeData[60] & 0xFF00) >> 8;
    if (KsTa > 127) {
        KsTa = KsTa - 256;
    }
    KsTa = KsTa / 8192.0f;

    mlx90640->KsTa = KsTa;
}

//------------------------------------------------------------------------------

void ExtractKsToParameters(uint16_t* eeData, paramsMLX90640* mlx90640)
{
    int32_t KsToScale;
    int8_t step;

    step = ((eeData[63] & 0x3000) >> 12) * 10;

    mlx90640->ct[0] = -40;
    mlx90640->ct[1] = 0;
    mlx90640->ct[2] = (eeData[63] & 0x00F0) >> 4;
    mlx90640->ct[3] = (eeData[63] & 0x0F00) >> 8;

    mlx90640->ct[2] = mlx90640->ct[2] * step;
    mlx90640->ct[3] = mlx90640->ct[2] + mlx90640->ct[3] * step;
    mlx90640->ct[4] = 400;

    KsToScale = (eeData[63] & 0x000F) + 8;
    KsToScale = 1UL << KsToScale;

    mlx90640->ksTo[0] = eeData[61] & 0x00FF;
    mlx90640->ksTo[1] = (eeData[61] & 0xFF00) >> 8;
    mlx90640->ksTo[2] = eeData[62] & 0x00FF;
    mlx90640->ksTo[3] = (eeData[62] & 0xFF00) >> 8;

    for (int i = 0; i < 4; i++) {
        if (mlx90640->ksTo[i] > 127) {
            mlx90640->ksTo[i] = mlx90640->ksTo[i] - 256;
        }
        mlx90640->ksTo[i] = mlx90640->ksTo[i] / KsToScale;
    }

    mlx90640->ksTo[4] = -0.0002;
}

//------------------------------------------------------------------------------

void ExtractAlphaParameters(uint16_t* eeData, paramsMLX90640* mlx90640)
{
    int accRow[24];
    int accColumn[32];
    int p = 0;
    int alphaRef;
    uint8_t alphaScale;
    uint8_t accRowScale;
    uint8_t accColumnScale;
    uint8_t accRemScale;
    float alphaTemp[768];
    float temp;

    accRemScale = eeData[32] & 0x000F;
    accColumnScale = (eeData[32] & 0x00F0) >> 4;
    accRowScale = (eeData[32] & 0x0F00) >> 8;
    alphaScale = ((eeData[32] & 0xF000) >> 12) + 30;
    alphaRef = eeData[33];

    for (int i = 0; i < 6; i++) {
        p = i * 4;
        accRow[p + 0] = (eeData[34 + i] & 0x000F);
        accRow[p + 1] = (eeData[34 + i] & 0x00F0) >> 4;
        accRow[p + 2] = (eeData[34 + i] & 0x0F00) >> 8;
        accRow[p + 3] = (eeData[34 + i] & 0xF000) >> 12;
    }

    for (int i = 0; i < 24; i++) {
        if (accRow[i] > 7) {
            accRow[i] = accRow[i] - 16;
        }
    }

    for (int i = 0; i < 8; i++) {
        p = i * 4;
        accColumn[p + 0] = (eeData[40 + i] & 0x000F);
        accColumn[p + 1] = (eeData[40 + i] & 0x00F0) >> 4;
        accColumn[p + 2] = (eeData[40 + i] & 0x0F00) >> 8;
        accColumn[p + 3] = (eeData[40 + i] & 0xF000) >> 12;
    }

    for (int i = 0; i < 32; i++) {
        if (accColumn[i] > 7) {
            accColumn[i] = accColumn[i] - 16;
        }
    }

    for (int i = 0; i < 24; i++) {
        for (int j = 0; j < 32; j++) {
            p = 32 * i + j;
            alphaTemp[p] = (eeData[64 + p] & 0x03F0) >> 4;
            if (alphaTemp[p] > 31) {
                alphaTemp[p] = alphaTemp[p] - 64;
            }
            alphaTemp[p] = alphaTemp[p] * (1 << accRemScale);
            alphaTemp[p] = (alphaRef + (accRow[i] << accRowScale) + (accColumn[j] << accColumnScale) + alphaTemp[p]);
            alphaTemp[p] = alphaTemp[p] / pow(2, (double)alphaScale);
            alphaTemp[p] = alphaTemp[p] - mlx90640->tgc * (mlx90640->cpAlpha[0] + mlx90640->cpAlpha[1]) / 2;
            alphaTemp[p] = SCALEALPHA / alphaTemp[p];
        }
    }

    temp = alphaTemp[0];
    for (int i = 1; i < 768; i++) {
        if (alphaTemp[i] > temp) {
            temp = alphaTemp[i];
        }
    }

    alphaScale = 0;
    while (temp < 32767.4) {
        temp = temp * 2;
        alphaScale = alphaScale + 1;
    }

    for (int i = 0; i < 768; i++) {
        temp = alphaTemp[i] * pow(2, (double)alphaScale);
        mlx90640->alpha[i] = (temp + 0.5);
    }

    mlx90640->alphaScale = alphaScale;
}

//------------------------------------------------------------------------------

void ExtractOffsetParameters(uint16_t* eeData, paramsMLX90640* mlx90640)
{
    int occRow[24];
    int occColumn[32];
    int p = 0;
    uint16_t offsetRef;
    uint8_t occRowScale;
    uint8_t occColumnScale;
    uint8_t occRemScale;

    occRemScale = (eeData[16] & 0x000F);
    occColumnScale = (eeData[16] & 0x00F0) >> 4;
    occRowScale = (eeData[16] & 0x0F00) >> 8;
    offsetRef = eeData[17];
    if (offsetRef > 32767) {
        offsetRef = offsetRef - 65536;
    }

    for (int i = 0; i < 6; i++) {
        p = i * 4;
        occRow[p + 0] = (eeData[18 + i] & 0x000F);
        occRow[p + 1] = (eeData[18 + i] & 0x00F0) >> 4;
        occRow[p + 2] = (eeData[18 + i] & 0x0F00) >> 8;
        occRow[p + 3] = (eeData[18 + i] & 0xF000) >> 12;
    }

    for (int i = 0; i < 24; i++) {
        if (occRow[i] > 7) {
            occRow[i] = occRow[i] - 16;
        }
    }

    for (int i = 0; i < 8; i++) {
        p = i * 4;
        occColumn[p + 0] = (eeData[24 + i] & 0x000F);
        occColumn[p + 1] = (eeData[24 + i] & 0x00F0) >> 4;
        occColumn[p + 2] = (eeData[24 + i] & 0x0F00) >> 8;
        occColumn[p + 3] = (eeData[24 + i] & 0xF000) >> 12;
    }

    for (int i = 0; i < 32; i++) {
        if (occColumn[i] > 7) {
            occColumn[i] = occColumn[i] - 16;
        }
    }

    for (int i = 0; i < 24; i++) {
        for (int j = 0; j < 32; j++) {
            p = 32 * i + j;
            mlx90640->offset[p] = (eeData[64 + p] & 0xFC00) >> 10;
            if (mlx90640->offset[p] > 31) {
                mlx90640->offset[p] = mlx90640->offset[p] - 64;
            }
            mlx90640->offset[p] = mlx90640->offset[p] * (1 << occRemScale);
            mlx90640->offset[p] = (offsetRef + (occRow[i] << occRowScale) + (occColumn[j] << occColumnScale) + mlx90640->offset[p]);
        }
    }
}

//------------------------------------------------------------------------------

void ExtractKtaPixelParameters(uint16_t* eeData, paramsMLX90640* mlx90640)
{
    int p = 0;
    int8_t KtaRC[4];
    uint8_t KtaRoCo;
    uint8_t KtaRoCe;
    uint8_t KtaReCo;
    uint8_t KtaReCe;
    uint8_t ktaScale1;
    uint8_t ktaScale2;
    uint8_t split;
    float ktaTemp[768];
    float temp;

    KtaRoCo = (eeData[54] & 0xFF00) >> 8;
    if (KtaRoCo > 127) {
        KtaRoCo = KtaRoCo - 256;
    }
    KtaRC[0] = KtaRoCo;

    KtaReCo = (eeData[54] & 0x00FF);
    if (KtaReCo > 127) {
        KtaReCo = KtaReCo - 256;
    }
    KtaRC[2] = KtaReCo;

    KtaRoCe = (eeData[55] & 0xFF00) >> 8;
    if (KtaRoCe > 127) {
        KtaRoCe = KtaRoCe - 256;
    }
    KtaRC[1] = KtaRoCe;

    KtaReCe = (eeData[55] & 0x00FF);
    if (KtaReCe > 127) {
        KtaReCe = KtaReCe - 256;
    }
    KtaRC[3] = KtaReCe;

    ktaScale1 = ((eeData[56] & 0x00F0) >> 4) + 8;
    ktaScale2 = (eeData[56] & 0x000F);

    for (int i = 0; i < 24; i++) {
        for (int j = 0; j < 32; j++) {
            p = 32 * i + j;
            split = 2 * (p / 32 - (p / 64) * 2) + p % 2;
            ktaTemp[p] = (eeData[64 + p] & 0x000E) >> 1;
            if (ktaTemp[p] > 3) {
                ktaTemp[p] = ktaTemp[p] - 8;
            }
            ktaTemp[p] = ktaTemp[p] * (1 << ktaScale2);
            ktaTemp[p] = KtaRC[split] + ktaTemp[p];
            ktaTemp[p] = ktaTemp[p] / pow(2, (double)ktaScale1);
            // ktaTemp[p] = ktaTemp[p] * mlx90640->offset[p];
        }
    }

    temp = fabs(ktaTemp[0]);
    for (int i = 1; i < 768; i++) {
        if (fabs(ktaTemp[i]) > temp) {
            temp = fabs(ktaTemp[i]);
        }
    }

    ktaScale1 = 0;
    while (temp < 63.4) {
        temp = temp * 2;
        ktaScale1 = ktaScale1 + 1;
    }

    for (int i = 0; i < 768; i++) {
        temp = ktaTemp[i] * pow(2, (double)ktaScale1);
        if (temp < 0) {
            mlx90640->kta[i] = (temp - 0.5);
        } else {
            mlx90640->kta[i] = (temp + 0.5);
        }
    }

    mlx90640->ktaScale = ktaScale1;
}

//------------------------------------------------------------------------------

void ExtractKvPixelParameters(uint16_t* eeData, paramsMLX90640* mlx90640)
{
    int p = 0;
    int8_t KvT[4];
    int8_t KvRoCo;
    int8_t KvRoCe;
    int8_t KvReCo;
    int8_t KvReCe;
    uint8_t kvScale;
    uint8_t split;
    float kvTemp[768];
    float temp;

    KvRoCo = (eeData[52] & 0xF000) >> 12;
    if (KvRoCo > 7) {
        KvRoCo = KvRoCo - 16;
    }
    KvT[0] = KvRoCo;

    KvReCo = (eeData[52] & 0x0F00) >> 8;
    if (KvReCo > 7) {
        KvReCo = KvReCo - 16;
    }
    KvT[2] = KvReCo;

    KvRoCe = (eeData[52] & 0x00F0) >> 4;
    if (KvRoCe > 7) {
        KvRoCe = KvRoCe - 16;
    }
    KvT[1] = KvRoCe;

    KvReCe = (eeData[52] & 0x000F);
    if (KvReCe > 7) {
        KvReCe = KvReCe - 16;
    }
    KvT[3] = KvReCe;

    kvScale = (eeData[56] & 0x0F00) >> 8;

    for (int i = 0; i < 24; i++) {
        for (int j = 0; j < 32; j++) {
            p = 32 * i + j;
            split = 2 * (p / 32 - (p / 64) * 2) + p % 2;
            kvTemp[p] = KvT[split];
            kvTemp[p] = kvTemp[p] / pow(2, (double)kvScale);
            // kvTemp[p] = kvTemp[p] * mlx90640->offset[p];
        }
    }

    temp = fabs(kvTemp[0]);
    for (int i = 1; i < 768; i++) {
        if (fabs(kvTemp[i]) > temp) {
            temp = fabs(kvTemp[i]);
        }
    }

    kvScale = 0;
    while (temp < 63.4) {
        temp = temp * 2;
        kvScale = kvScale + 1;
    }

    for (int i = 0; i < 768; i++) {
        temp = kvTemp[i] * pow(2, (double)kvScale);
        if (temp < 0) {
            mlx90640->kv[i] = (temp - 0.5);
        } else {
            mlx90640->kv[i] = (temp + 0.5);
        }
    }

    mlx90640->kvScale = kvScale;
}

//------------------------------------------------------------------------------

void ExtractCPParameters(uint16_t* eeData, paramsMLX90640* mlx90640)
{
    float alphaSP[2];
    int16_t offsetSP[2];
    float cpKv;
    float cpKta;
    uint8_t alphaScale;
    uint8_t ktaScale1;
    uint8_t kvScale;

    alphaScale = ((eeData[32] & 0xF000) >> 12) + 27;

    offsetSP[0] = (eeData[58] & 0x03FF);
    if (offsetSP[0] > 511) {
        offsetSP[0] = offsetSP[0] - 1024;
    }

    offsetSP[1] = (eeData[58] & 0xFC00) >> 10;
    if (offsetSP[1] > 31) {
        offsetSP[1] = offsetSP[1] - 64;
    }
    offsetSP[1] = offsetSP[1] + offsetSP[0];

    alphaSP[0] = (eeData[57] & 0x03FF);
    if (alphaSP[0] > 511) {
        alphaSP[0] = alphaSP[0] - 1024;
    }
    alphaSP[0] = alphaSP[0] / pow(2, (double)alphaScale);

    alphaSP[1] = (eeData[57] & 0xFC00) >> 10;
    if (alphaSP[1] > 31) {
        alphaSP[1] = alphaSP[1] - 64;
    }
    alphaSP[1] = (1 + alphaSP[1] / 128) * alphaSP[0];

    cpKta = (eeData[59] & 0x00FF);
    if (cpKta > 127) {
        cpKta = cpKta - 256;
    }
    ktaScale1 = ((eeData[56] & 0x00F0) >> 4) + 8;
    mlx90640->cpKta = cpKta / pow(2, (double)ktaScale1);

    cpKv = (eeData[59] & 0xFF00) >> 8;
    if (cpKv > 127) {
        cpKv = cpKv - 256;
    }
    kvScale = (eeData[56] & 0x0F00) >> 8;
    mlx90640->cpKv = cpKv / pow(2, (double)kvScale);

    mlx90640->cpAlpha[0] = alphaSP[0];
    mlx90640->cpAlpha[1] = alphaSP[1];
    mlx90640->cpOffset[0] = offsetSP[0];
    mlx90640->cpOffset[1] = offsetSP[1];
}

//------------------------------------------------------------------------------

void ExtractCILCParameters(uint16_t* eeData, paramsMLX90640* mlx90640)
{
    float ilChessC[3];
    uint8_t calibrationModeEE;

    calibrationModeEE = (eeData[10] & 0x0800) >> 4;
    calibrationModeEE = calibrationModeEE ^ 0x80;

    ilChessC[0] = (eeData[53] & 0x003F);
    if (ilChessC[0] > 31) {
        ilChessC[0] = ilChessC[0] - 64;
    }
    ilChessC[0] = ilChessC[0] / 16.0f;

    ilChessC[1] = (eeData[53] & 0x07C0) >> 6;
    if (ilChessC[1] > 15) {
        ilChessC[1] = ilChessC[1] - 32;
    }
    ilChessC[1] = ilChessC[1] / 2.0f;

    ilChessC[2] = (eeData[53] & 0xF800) >> 11;
    if (ilChessC[2] > 15) {
        ilChessC[2] = ilChessC[2] - 32;
    }
    ilChessC[2] = ilChessC[2] / 8.0f;

    mlx90640->calibrationModeEE = calibrationModeEE;
    mlx90640->ilChessC[0] = ilChessC[0];
    mlx90640->ilChessC[1] = ilChessC[1];
    mlx90640->ilChessC[2] = ilChessC[2];
}

//------------------------------------------------------------------------------

int ExtractDeviatingPixels(uint16_t* eeData, paramsMLX90640* mlx90640)
{
    uint16_t pixCnt = 0;
    uint16_t brokenPixCnt = 0;
    uint16_t outlierPixCnt = 0;
    int warn = 0;
    int i;

    for (pixCnt = 0; pixCnt < 5; pixCnt++) {
        mlx90640->brokenPixels[pixCnt] = 0xFFFF;
        mlx90640->outlierPixels[pixCnt] = 0xFFFF;
    }

    pixCnt = 0;
    while (pixCnt < 768 && brokenPixCnt < 5 && outlierPixCnt < 5) {
        if (eeData[pixCnt + 64] == 0) {
            mlx90640->brokenPixels[brokenPixCnt] = pixCnt;
            brokenPixCnt = brokenPixCnt + 1;
        } else if ((eeData[pixCnt + 64] & 0x0001) != 0) {
            mlx90640->outlierPixels[outlierPixCnt] = pixCnt;
            outlierPixCnt = outlierPixCnt + 1;
        }

        pixCnt = pixCnt + 1;
    }

    if (brokenPixCnt > 4) {
        warn = -3;
    } else if (outlierPixCnt > 4) {
        warn = -4;
    } else if ((brokenPixCnt + outlierPixCnt) > 4) {
        warn = -5;
    } else {
        for (pixCnt = 0; pixCnt < brokenPixCnt; pixCnt++) {
            for (i = pixCnt + 1; i < brokenPixCnt; i++) {
                warn = CheckAdjacentPixels(mlx90640->brokenPixels[pixCnt], mlx90640->brokenPixels[i]);
                if (warn != 0) {
                    return warn;
                }
            }
        }

        for (pixCnt = 0; pixCnt < outlierPixCnt; pixCnt++) {
            for (i = pixCnt + 1; i < outlierPixCnt; i++) {
                warn = CheckAdjacentPixels(mlx90640->outlierPixels[pixCnt], mlx90640->outlierPixels[i]);
                if (warn != 0) {
                    return warn;
                }
            }
        }

        for (pixCnt = 0; pixCnt < brokenPixCnt; pixCnt++) {
            for (i = 0; i < outlierPixCnt; i++) {
                warn = CheckAdjacentPixels(mlx90640->brokenPixels[pixCnt], mlx90640->outlierPixels[i]);
                if (warn != 0) {
                    return warn;
                }
            }
        }
    }

    return warn;
}

//------------------------------------------------------------------------------

/**
 * @brief
 *
 * @param pix1
 * @param pix2
 * @return int
 */
int CheckAdjacentPixels(uint16_t pix1, uint16_t pix2)
{
    int pixPosDif;

    pixPosDif = pix1 - pix2;
    if (pixPosDif > -34 && pixPosDif < -30) {
        return -6;
    }
    if (pixPosDif > -2 && pixPosDif < 2) {
        return -6;
    }
    if (pixPosDif > 30 && pixPosDif < 34) {
        return -6;
    }

    return 0;
}

//------------------------------------------------------------------------------

float GetMedian(float* values, int n)
{
    float temp;

    for (int i = 0; i < n - 1; i++) {
        for (int j = i + 1; j < n; j++) {
            if (values[j] < values[i]) {
                temp = values[i];
                values[i] = values[j];
                values[j] = temp;
            }
        }
    }

    if (n % 2 == 0) {
        return ((values[n / 2] + values[n / 2 - 1]) / 2.0);

    } else {
        return values[n / 2];
    }
}

//------------------------------------------------------------------------------

/**
 * @brief 判断损坏的像素
 *
 * @param pixel
 * @param params
 * @return int
 */
int IsPixelBad(uint16_t pixel, paramsMLX90640* params)
{
    for (int i = 0; i < 5; i++) {
        if (pixel == params->outlierPixels[i] || pixel == params->brokenPixels[i]) {
            return 1;
        }
    }

    return 0;
}

//------------------------------------------------------------------------------
