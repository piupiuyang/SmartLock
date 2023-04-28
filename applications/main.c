/*
 * Copyright (c) 2006-2023, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2023-02-23     RT-Thread    first version
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include "aht10.h"
#include "ssd1306_tests.h"
#include "fpm383c.h"
#define DBG_TAG "main"
#define DBG_LVL DBG_LOG
#include "ttp229.h"
#define LED_GREEN_PIN  GET_PIN(B,0)
#define LED_BLUE_PIN  GET_PIN(B,1)
#define LED_CONF(LED_PIN)  rt_pin_mode(LED_PIN, PIN_MODE_OUTPUT)
#define LED_OFF(PIN)  rt_pin_write(PIN, 1)
#define LED_ON(PIN)  rt_pin_write(PIN, 0)
#include <rtdbg.h>


int main(void)
{
    LED_CONF(LED_GREEN_PIN);
    LED_CONF(LED_BLUE_PIN);
    LED_OFF(LED_GREEN_PIN);
    LED_OFF(LED_BLUE_PIN);
    ssd1306_Init();
    //ssd1306_TestAll();

    thread_aht10_init();

    thread_keyhandle_init();

    thread_fpm383_init();
    //ssd1306_TestFonts();
    //unsigned int i = 0;
//    i = HAL_RCC_GetSysClockFreq();
/*    while(1)
    {

        //LOG_D("Hello RT-Thread!%d",i);
        FPM383C_LED_ON(3, 2000);
        rt_thread_mdelay(1000);
        //if(ttp29_GetKey() == '1')rt_pin_write(LED_GREEN_PIN, i^1);
        //LOG_D("Hello RT-Thread!%d",i);
        //LOG_D("Hello RT-Thread!%d",i);
        //LOG_D("Hello RT-Thread!%d",i);
    }*/

    LOG_D("Hello RT-Thread!%d",HAL_RCC_GetSysClockFreq());

    return RT_EOK;
}
