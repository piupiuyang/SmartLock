/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2023-03-20     piupiuY       the first version
 */
#ifndef APPLICATIONS_AHT10_AHT10_H_
#define APPLICATIONS_AHT10_AHT10_H_

#include <rtthread.h>
#include <rtdevice.h>
#include "ssd1306.h"
#include <string.h>
#include <stdio.h>

#define AHT10_I2C_BUS_NAME          "i2c1"  /* 传感器连接的I2C总线设备名称 */
#define AHT10_ADDR                  0x38    /* 从机地址 */
#define AHT10_CALIBRATION_CMD       0xE1    /* 校准命令 */
#define AHT10_NORMAL_CMD            0xA8    /* 一般命令 */
#define AHT10_GET_DATA              0xAC    /* 获取数据命令 */

void i2c_aht10_take_data();
int thread_aht10_init();

#endif /* APPLICATIONS_AHT10_AHT10_H_ */
