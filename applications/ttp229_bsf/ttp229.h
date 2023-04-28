/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2023-03-21     piupiuY       the first version
 */
#ifndef APPLICATIONS_TTP229_BSF_TTP229_H_
#define APPLICATIONS_TTP229_BSF_TTP229_H_

#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include "ssd1306.h"
#include "beep.h"
#include <string.h>
#include <stdio.h>
#include "public_variables.h"
#define BEEP_PIN  GET_PIN(A,8)
#define BEEP_ON  rt_pin_write(BEEP_PIN, PIN_HIGH);
#define BEEP_OFF rt_pin_write(BEEP_PIN, PIN_LOW);
#define SCL_PIN GET_PIN(B,9)
#define SDO_PIN GET_PIN(B,8)

#define KEY_MAX 16

typedef struct {
    char keyname;
    rt_uint16_t keycode;
}KEY_MAP;



void singelkey_get(rt_uint16_t KeyCode);
int thread_keyhandle_init();


#endif /* APPLICATIONS_TTP229_BSF_TTP229_H_ */
