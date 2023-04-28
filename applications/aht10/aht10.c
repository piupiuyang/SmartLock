/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2023-03-20     piupiuY       the first version
 */
#include "aht10.h"
#define DBG_TAG "aht10"
#define DBG_LVL DBG_LOG
#define THREAD_AHT10_PRIORITY 25
#define THREAD_AHT10_STACK_SIZE 128
#define THREAD_AHT10_TIMESLICE 5
#include <rtdbg.h>
static struct rt_i2c_bus_device *i2c_bus = RT_NULL;     /* I2C总线设备句柄 */
static rt_bool_t initialized = RT_FALSE;                /* 传感器初始化状态 */

ALIGN(RT_ALIGN_SIZE)
static char thread_aht10_stack[1024];
static struct rt_thread thread_aht10;
float humidity = 0.0, temperature = 0.0;
char str_humidity[5],str_temperature[5];


/* 写传感器寄存器 */
static rt_err_t write_reg(struct rt_i2c_bus_device *bus, rt_uint8_t reg, rt_uint8_t *data)
{
    rt_uint8_t buf[3];
    struct rt_i2c_msg msgs;

    buf[0] = reg; //cmd
    buf[1] = data[0];
    buf[2] = data[1];

    msgs.addr = AHT10_ADDR;
    msgs.flags = RT_I2C_WR;
    msgs.buf = buf;
    msgs.len = 3;

    /* 调用I2C设备接口传输数据 */
    if (rt_i2c_transfer(bus, &msgs, 1) == 1)
    {
        return RT_EOK;
    }
    else
    {
        return -RT_ERROR;
    }
}

/* 读传感器寄存器数据 */
static rt_err_t read_regs(struct rt_i2c_bus_device *bus, rt_uint8_t len, rt_uint8_t *buf)
{
    struct rt_i2c_msg msgs;

    msgs.addr = AHT10_ADDR;
    msgs.flags = RT_I2C_RD;
    msgs.buf = buf;
    msgs.len = len;

    /* 调用I2C设备接口传输数据 */
    if (rt_i2c_transfer(bus, &msgs, 1) == 1)
    {
        return RT_EOK;
    }
    else
    {
        return -RT_ERROR;
    }
}

static void read_temp_humi(float *cur_temp, float *cur_humi)
{
    rt_uint8_t temp[6];

    write_reg(i2c_bus, AHT10_GET_DATA, 0);      /* 发送命令 */
    read_regs(i2c_bus, 6, temp);                /* 获取传感器数据 */

    /* 湿度数据转换 */
    *cur_humi = (temp[1] << 12 | temp[2] << 4 | (temp[3] & 0xf0) >> 4) * 100.0 / (1 << 20);
    /* 温度数据转换 */
    *cur_temp = ((temp[3] & 0xf) << 16 | temp[4] << 8 | temp[5]) * 200.0 / (1 << 20) - 50;
}

static void aht10_init(const char *name)
{
    rt_uint8_t temp[2] = {0, 0};

    /* 查找I2C总线设备，获取I2C总线设备句柄 */
    i2c_bus = (struct rt_i2c_bus_device *)rt_device_find(name);

    if (i2c_bus == RT_NULL)
    {
        LOG_D("can't find %s device!\n", name);
    }
    else
    {
        write_reg(i2c_bus, AHT10_NORMAL_CMD, temp);
        rt_thread_mdelay(400);

        temp[0] = 0x08;
        temp[1] = 0x00;
        write_reg(i2c_bus, AHT10_CALIBRATION_CMD, temp);
        rt_thread_mdelay(400);
        initialized = RT_TRUE;
    }
    memset(str_humidity,0,sizeof(str_humidity));
    memset(str_temperature,0,sizeof(str_temperature));
}

void i2c_aht10_take_data(void){
    if (!initialized)
       {
           /* 传感器初始化 */
           aht10_init(AHT10_I2C_BUS_NAME);
       }
       if (initialized)
       {
           /* 读取温湿度数据 */
           read_temp_humi(&temperature, &humidity);
           sprintf(str_humidity,"%.1f",humidity);
           sprintf(str_temperature,"%.1f",temperature);
           //ssd1306_Fill(Black);
           ssd1306_SetCursor(0, 0);
           ssd1306_WriteString(str_humidity, Font_7x10, White);
           ssd1306_SetCursor(0, 10);
           ssd1306_WriteString(str_temperature, Font_7x10, White);
           ssd1306_UpdateScreen();
         /*  rt_kprintf("read aht10 sensor humidity   : %d.%d %%\n", (int)humidity, (int)(humidity * 10) % 10);
           rt_kprintf("read aht10 sensor temperature: %d.%d \n", (int)temperature, (int)(temperature * 10) % 10);*/
       }

}

static void thread_aht10_entry(){

    while(1){
        i2c_aht10_take_data();
        rt_thread_mdelay(3000);
    }

}

int thread_aht10_init(){
    aht10_init(AHT10_I2C_BUS_NAME);
    rt_thread_init(&thread_aht10,
                   "td_aht10",
                   thread_aht10_entry,
                   RT_NULL,
                   &thread_aht10_stack,
                   sizeof(thread_aht10_stack),
                   THREAD_AHT10_PRIORITY, THREAD_AHT10_TIMESLICE);

    rt_thread_startup(&thread_aht10);

    return 0;
}
static void i2c_aht10_sample(int argc, char *argv[])
{
    float humidity, temperature;
    char name[RT_NAME_MAX];
    char str_humidity[5],str_temperature[5];
    humidity = 0.0;
    temperature = 0.0;
    memset(str_humidity,0,sizeof(str_humidity));
    memset(str_temperature,0,sizeof(str_temperature));
    if (argc == 2)
    {
        rt_strncpy(name, argv[1], RT_NAME_MAX);
    }
    else
    {
        rt_strncpy(name, AHT10_I2C_BUS_NAME, RT_NAME_MAX);
    }

    if (!initialized)
    {
        /* 传感器初始化 */
        aht10_init(name);
    }
    if (initialized)
    {
        /* 读取温湿度数据 */
        read_temp_humi(&temperature, &humidity);
        sprintf(str_humidity,"%.1f",humidity);
        sprintf(str_temperature,"%.1f",temperature);
        ssd1306_Fill(Black);
        ssd1306_SetCursor(0, 0);
        ssd1306_WriteString(str_humidity, Font_7x10, White);
        ssd1306_SetCursor(0, 10);
        ssd1306_WriteString(str_temperature, Font_7x10, White);
        ssd1306_UpdateScreen();
        rt_kprintf("read aht10 sensor humidity   : %d.%d %%\n", (int)humidity, (int)(humidity * 10) % 10);
        rt_kprintf("read aht10 sensor temperature: %d.%d \n", (int)temperature, (int)(temperature * 10) % 10);
    }
    else
    {
        rt_kprintf("initialize sensor failed!\n");
    }
}
/* 导出到 msh 命令列表中 */
MSH_CMD_EXPORT(i2c_aht10_sample, i2c aht10 sample);
