/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2023-03-23     piupiuY       the first version
 */
#ifndef APPLICATIONS_FPM383C_FPM383C_H_
#define APPLICATIONS_FPM383C_FPM383C_H_

#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include <string.h>
#include <stdio.h>
#include "public_variables.h"

#define TOUCHOUT_PIN  GET_PIN(A,11)


void fpm383c_uart_and_touchPin_Init();
void FPM383C_SendData(int length,rt_uint8_t FPM383C_Databuffer[]);
rt_uint8_t FPM383C_ControlLED(rt_uint8_t PS_ControlLEDBuffer[],rt_uint16_t Timeout);
void FPM383C_LED_ON(rt_uint8_t LEDnumber ,rt_uint16_t Time );

void print_usart2_buffer(rt_uint8_t USART2_ReceiveBuffer[20]);
void FPM383C_Sleep(void);
int thread_fpm383_init();
#endif /* APPLICATIONS_FPM383C_FPM383C_H_ */
