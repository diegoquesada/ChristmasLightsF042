/*
 * light_sensor.cpp
 *
 *  Created on: Dec 13, 2021
 *      Author: Diego Quesada
 *
 *  Substantially based on Grove Digital Light Sensor Library at:
 *    https://github.com/Seeed-Studio/Grove_Digital_Light_Sensor
 *
 * Copyright (c) 2013 Seeed Technology Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
*/

#include "stm32f0xx_hal.h"
#include "light_sensor.h"

#define TSL2561_Address  0x29 << 1

#define  TSL2561_Control  0x80
#define  TSL2561_Timing   0x81
#define  TSL2561_Interrupt 0x86
#define  TSL2561_Channal0L 0x8C
#define  TSL2561_Channal0H 0x8D
#define  TSL2561_Channal1L 0x8E
#define  TSL2561_Channal1H 0x8F

#define LUX_SCALE 14           // scale by 2^14
#define RATIO_SCALE 9          // scale ratio by 2^9
#define CH_SCALE 10            // scale channel values by 2^10
#define CHSCALE_TINT0 0x7517   // 322/11 * 2^CH_SCALE
#define CHSCALE_TINT1 0x0fe7   // 322/81 * 2^CH_SCALE

#define K1T 0x0040   // 0.125 * 2^RATIO_SCALE
#define B1T 0x01f2   // 0.0304 * 2^LUX_SCALE
#define M1T 0x01be   // 0.0272 * 2^LUX_SCALE
#define K2T 0x0080   // 0.250 * 2^RATIO_SCA
#define B2T 0x0214   // 0.0325 * 2^LUX_SCALE
#define M2T 0x02d1   // 0.0440 * 2^LUX_SCALE
#define K3T 0x00c0   // 0.375 * 2^RATIO_SCALE
#define B3T 0x023f   // 0.0351 * 2^LUX_SCALE
#define M3T 0x037b   // 0.0544 * 2^LUX_SCALE
#define K4T 0x0100   // 0.50 * 2^RATIO_SCALE
#define B4T 0x0270   // 0.0381 * 2^LUX_SCALE
#define M4T 0x03fe   // 0.0624 * 2^LUX_SCALE
#define K5T 0x0138   // 0.61 * 2^RATIO_SCALE
#define B5T 0x016f   // 0.0224 * 2^LUX_SCALE
#define M5T 0x01fc   // 0.0310 * 2^LUX_SCALE
#define K6T 0x019a   // 0.80 * 2^RATIO_SCALE
#define B6T 0x00d2   // 0.0128 * 2^LUX_SCALE
#define M6T 0x00fb   // 0.0153 * 2^LUX_SCALE
#define K7T 0x029a   // 1.3 * 2^RATIO_SCALE
#define B7T 0x0018   // 0.00146 * 2^LUX_SCALE
#define M7T 0x0012   // 0.00112 * 2^LUX_SCALE
#define K8T 0x029a   // 1.3 * 2^RATIO_SCALE
#define B8T 0x0000   // 0.000 * 2^LUX_SCALE
#define M8T 0x0000   // 0.000 * 2^LUX_SCALE



#define K1C 0x0043   // 0.130 * 2^RATIO_SCALE
#define B1C 0x0204   // 0.0315 * 2^LUX_SCALE
#define M1C 0x01ad   // 0.0262 * 2^LUX_SCALE
#define K2C 0x0085   // 0.260 * 2^RATIO_SCALE
#define B2C 0x0228   // 0.0337 * 2^LUX_SCALE
#define M2C 0x02c1   // 0.0430 * 2^LUX_SCALE
#define K3C 0x00c8   // 0.390 * 2^RATIO_SCALE
#define B3C 0x0253   // 0.0363 * 2^LUX_SCALE
#define M3C 0x0363   // 0.0529 * 2^LUX_SCALE
#define K4C 0x010a   // 0.520 * 2^RATIO_SCALE
#define B4C 0x0282   // 0.0392 * 2^LUX_SCALE
#define M4C 0x03df   // 0.0605 * 2^LUX_SCALE
#define K5C 0x014d   // 0.65 * 2^RATIO_SCALE
#define B5C 0x0177   // 0.0229 * 2^LUX_SCALE
#define M5C 0x01dd   // 0.0291 * 2^LUX_SCALE
#define K6C 0x019a   // 0.80 * 2^RATIO_SCALE
#define B6C 0x0101   // 0.0157 * 2^LUX_SCALE
#define M6C 0x0127   // 0.0180 * 2^LUX_SCALE
#define K7C 0x029a   // 1.3 * 2^RATIO_SCALE
#define B7C 0x0037   // 0.00338 * 2^LUX_SCALE
#define M7C 0x002b   // 0.00260 * 2^LUX_SCALE
#define K8C 0x029a   // 1.3 * 2^RATIO_SCALE
#define B8C 0x0000   // 0.000 * 2^LUX_SCALE
#define M8C 0x0000   // 0.000 * 2^LUX_SCALE


extern I2C_HandleTypeDef *g_hi2c1;

uint8_t CH0_LOW, CH0_HIGH, CH1_LOW, CH1_HIGH;
uint16_t ch0, ch1;

unsigned long chScale;
unsigned long channel1;
unsigned long channel0;
unsigned long  ratio1;
unsigned int b;
unsigned int m;
unsigned long temp;
unsigned long lux;

uint8_t light_readRegister(int deviceAddress, int address)
{
	uint8_t buf[16];
	HAL_StatusTypeDef status = HAL_OK;

	buf[0] = address;
	status = HAL_I2C_Master_Transmit(g_hi2c1, deviceAddress, buf, 1, HAL_MAX_DELAY);
	if (status == HAL_OK)
	{
		status = HAL_I2C_Master_Receive(g_hi2c1, deviceAddress, buf, 1, HAL_MAX_DELAY);
		return buf[0];
	}
	else
		return 0;
}

void light_writeRegister(int deviceAddress, int address, uint8_t val)
{
	uint8_t buf[16];
	HAL_StatusTypeDef status = HAL_OK;

	buf[0] = address;
	buf[1] = val;
	status = HAL_I2C_Master_Transmit(g_hi2c1, deviceAddress, buf, 2, HAL_MAX_DELAY);
    //delay(100);
}

void light_getLux(void)
{
    CH0_LOW = light_readRegister(TSL2561_Address, TSL2561_Channal0L);
    CH0_HIGH = light_readRegister(TSL2561_Address, TSL2561_Channal0H);
    //read two bytes from registers 0x0E and 0x0F
    CH1_LOW = light_readRegister(TSL2561_Address, TSL2561_Channal1L);
    CH1_HIGH = light_readRegister(TSL2561_Address, TSL2561_Channal1H);

    ch0 = (CH0_HIGH << 8) | CH0_LOW;
    ch1 = (CH1_HIGH << 8) | CH1_LOW;
}

void light_init()
{
	light_writeRegister(TSL2561_Address, TSL2561_Control, 0x03); // POWER UP
	light_writeRegister(TSL2561_Address, TSL2561_Timing, 0x00); //No High Gain (1x), integration time of 13ms
	light_writeRegister(TSL2561_Address, TSL2561_Interrupt, 0x00);
	light_writeRegister(TSL2561_Address, TSL2561_Control, 0x00); // POWER Down
}

unsigned long light_calculateLux(unsigned int iGain, unsigned int tInt, int iType)
{
    switch (tInt)
    {
        case 0:  // 13.7 msec
            chScale = CHSCALE_TINT0;
            break;
        case 1: // 101 msec
            chScale = CHSCALE_TINT1;
            break;
        default: // assume no scaling
            chScale = (1 << CH_SCALE);
            break;
    }
    if (!iGain)
    {
        chScale = chScale << 4;    // scale 1X to 16X
    }
    // scale the channel values
    channel0 = (ch0 * chScale) >> CH_SCALE;
    channel1 = (ch1 * chScale) >> CH_SCALE;

    ratio1 = 0;
    if (channel0 != 0)
    {
        ratio1 = (channel1 << (RATIO_SCALE + 1)) / channel0;
    }
    // round the ratio value
    unsigned long ratio = (ratio1 + 1) >> 1;

    switch (iType)
    {
        case 0: // T package
            if ((ratio >= 0) && (ratio <= K1T)) {
                b = B1T;
                m = M1T;
            } else if (ratio <= K2T) {
                b = B2T;
                m = M2T;
            } else if (ratio <= K3T) {
                b = B3T;
                m = M3T;
            } else if (ratio <= K4T) {
                b = B4T;
                m = M4T;
            } else if (ratio <= K5T) {
                b = B5T;
                m = M5T;
            } else if (ratio <= K6T) {
                b = B6T;
                m = M6T;
            } else if (ratio <= K7T) {
                b = B7T;
                m = M7T;
            } else if (ratio > K8T) {
                b = B8T;
                m = M8T;
            }
            break;
        case 1:// CS package
            if ((ratio >= 0) && (ratio <= K1C)) {
                b = B1C;
                m = M1C;
            } else if (ratio <= K2C) {
                b = B2C;
                m = M2C;
            } else if (ratio <= K3C) {
                b = B3C;
                m = M3C;
            } else if (ratio <= K4C) {
                b = B4C;
                m = M4C;
            } else if (ratio <= K5C) {
                b = B5C;
                m = M5C;
            } else if (ratio <= K6C) {
                b = B6C;
                m = M6C;
            } else if (ratio <= K7C) {
                b = B7C;
                m = M7C;
            }
    }
    temp = ((channel0 * b) - (channel1 * m));
    if (temp < 0)
    {
        temp = 0;
    }
    temp += (1 << (LUX_SCALE - 1));
    // strip off fractional portion
    lux = temp >> LUX_SCALE;
    return (lux);
}

signed long light_readVisibleLux()
{
	light_writeRegister(TSL2561_Address, TSL2561_Control, 0x03); // POWER UP
    HAL_Delay(14);
    light_getLux();

    light_writeRegister(TSL2561_Address, TSL2561_Control, 0x00); // POWER Down
    if (ch1 == 0)
    {
        return 0;
    }
    if (ch0 / ch1 < 2 && ch0 > 4900)
    {
        return -1;  //ch0 out of range, but ch1 not. the lux is not valid in this situation.
    }
    return light_calculateLux(0, 0, 0);  //T package, no gain, 13ms
}
