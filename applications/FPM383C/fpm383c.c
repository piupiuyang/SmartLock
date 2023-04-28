/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2023-03-23     piupiuY       the first version
 */
#include "fpm383c.h"
#define DBG_TAG "fpm383"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

#define FPM383C_UART_NAME "uart2" /* 串口设备名称 */

static rt_device_t serial;/* 串口设备句柄 */
struct serial_configure config = RT_SERIAL_CONFIG_DEFAULT;  /* 初始化配置参数 */


#define THREAD_FPM383_PRIORITY 19
#define THREAD_KFPM383_TIMESLICE 50
#define FINGERPRINT_IDSET_MODE 0
#define FINGERPRINT_IDDELETE_MODE 1
#define FINGERPRINT_IDEMPTY_MODE 2


/* 用于接收消息的信号量 */
//static struct rt_semaphore rx_sem;

rt_sem_t fingerprint_set_state_over_sem = RT_NULL;

//USART串口接收长度以及标志位
rt_uint8_t USART2_STA  = 0;
//rt_uint8_t USART3_STA  = 0;

//指纹ID和验证指纹的分数
rt_uint8_t PageID,MatchScore;

rt_uint8_t FingerPrint_Number;
//USART串口接收缓冲数组
rt_uint8_t USART2_ReceiveBuffer[20];


//主循环状态标志位
rt_uint8_t ScanStatus = 0;

//三种模式对应的命令
const char StringEnroll[7]  = "Enroll";
const char StringEmpty[7]   = "Empty";
const char StringDelete[7]  = "Delete";

//模块LED灯控制协议
rt_uint8_t PS_BlueLEDBuffer[16] = {0xEF,0x01,0xFF,0xFF,0xFF,0xFF,0x01,0x00,0x07,0x3C,0x03,0x01,0x01,0x00,0x00,0x49};
rt_uint8_t PS_RedLEDBuffer[16] = {0xEF,0x01,0xFF,0xFF,0xFF,0xFF,0x01,0x00,0x07,0x3C,0x02,0x04,0x04,0x02,0x00,0x50};
rt_uint8_t PS_GreenLEDBuffer[16] = {0xEF,0x01,0xFF,0xFF,0xFF,0xFF,0x01,0x00,0x07,0x3C,0x02,0x02,0x02,0x02,0x00,0x4C};

//模块睡眠协议
rt_uint8_t PS_SleepBuffer[12] = {0xEF,0x01,0xFF,0xFF,0xFF,0xFF,0x01,0x00,0x03,0x33,0x00,0x37};

//清空指纹协议
rt_uint8_t PS_EmptyBuffer[12] = {0xEF,0x01,0xFF,0xFF,0xFF,0xFF,0x01,0x00,0x03,0x0D,0x00,0x11};

//取消命令协议
rt_uint8_t PS_CancelBuffer[12] = {0xEF,0x01,0xFF,0xFF,0xFF,0xFF,0x01,0x00,0x03,0x30,0x00,0x34};

//自动注册指纹协议
rt_uint8_t PS_AutoEnrollBuffer[17] = {0xEF,0x01,0xFF,0xFF,0xFF,0xFF,0x01,0x00,0x08,0x31,'\0','\0',0x04,0x00,0x16,'\0','\0'}; //PageID: bit 10:11，SUM: bit 15:16

//模块搜索用获取图像协议
rt_uint8_t PS_GetImageBuffer[12] = {0xEF,0x01,0xFF,0xFF,0xFF,0xFF,0x01,0x00,0x03,0x01,0x00,0x05};

//模块生成模板协议
rt_uint8_t PS_GetChar1Buffer[13] = {0xEF,0x01,0xFF,0xFF,0xFF,0xFF,0x01,0x00,0x04,0x02,0x01,0x00,0x08};
rt_uint8_t PS_GetChar2Buffer[13] = {0xEF,0x01,0xFF,0xFF,0xFF,0xFF,0x01,0x00,0x04,0x02,0x02,0x00,0x09};

//模块搜索指纹功能协议
rt_uint8_t PS_SearchMBBuffer[17] = {0xEF,0x01,0xFF,0xFF,0xFF,0xFF,0x01,0x00,0x08,0x04,0x01,0x00,0x00,0xFF,0xFF,0x02,0x0C};

//删除指纹协议
rt_uint8_t PS_DeleteBuffer[16] = {0xEF,0x01,0xFF,0xFF,0xFF,0xFF,0x01,0x00,0x07,0x0C,'\0','\0',0x00,0x01,'\0','\0'}; //PageID: bit 10:11，SUM: bit 14:15

/* 接收数据回调函数 */
void touch_pin_irqhandle(){

    ScanStatus |= 1<<7;
    LOG_D("ScanStatus:0x%x",ScanStatus);
}

static rt_err_t uart_input(rt_device_t dev, rt_size_t size)
{
    rt_uint8_t res;
    /* 串口接收到数据后产生中断，调用此回调函数，然后发送接收信号量 */
    rt_device_read(serial,0,&res,1);
    if((USART2_STA & 0x80) == 0)
    {
        if(USART2_STA < 20)
        {
            USART2_ReceiveBuffer[USART2_STA++] = res;
        }
        else
        {
            USART2_STA |= 0x80;
        }
    }
    //rt_sem_release(&rx_sem);
    return RT_EOK;
}
void print_usart2_buffer(rt_uint8_t USART2_ReceiveBuffer[20]){
    for(rt_uint8_t i = 0; i < 20; i++){

        LOG_D("USART2_ReceiveBuffer[%d]:0x%x",i,USART2_ReceiveBuffer[i]);

    }
}

void fpm383c_uart_and_touchPin_Init(){
    serial = rt_device_find(FPM383C_UART_NAME);
    /* step2：修改串口配置参数 */
    config.baud_rate = BAUD_RATE_57600;        //修改波特率为 57600
    config.data_bits = DATA_BITS_8;           //数据位 8
    config.stop_bits = STOP_BITS_1;           //停止位 1
    config.bufsz     = 128;                   //修改缓冲区 buff size 为 128
    config.parity    = PARITY_NONE;           //无奇偶校验位

    rt_device_control(serial, RT_DEVICE_CTRL_CONFIG, &config);
    rt_device_open(serial, RT_DEVICE_OFLAG_RDWR|RT_DEVICE_FLAG_INT_RX);
    rt_device_set_rx_indicate(serial, uart_input);
    rt_pin_mode(TOUCHOUT_PIN, PIN_MODE_INPUT_PULLDOWN);
    rt_pin_attach_irq(TOUCHOUT_PIN, PIN_IRQ_MODE_RISING, touch_pin_irqhandle, RT_NULL);
    rt_pin_irq_enable(TOUCHOUT_PIN, PIN_IRQ_ENABLE);
    FPM383C_ControlLED(PS_GreenLEDBuffer,2000);
    FPM383C_Sleep();

}
/**
    * @brief    USART2串口发送函数
    * @param    length: 发送数组长度
    * @param    FPM383C_Databuffer[]: 需要发送的功能协议数组，在上面已有定义
    * @return None
    */
void FPM383C_SendData(int length,rt_uint8_t FPM383C_Databuffer[])
{
    for(int i = 0;i<length;i++)
    {
        rt_device_write(serial, 0, &FPM383C_Databuffer[i], 1);
        //USART_SendData(USART2,FPM383C_Databuffer[i]);
        //while(!USART_GetFlagStatus(USART2,USART_FLAG_TXE));
    }
    USART2_STA = 0;
}

/**
    * @brief    发送睡眠指令，只有发送睡眠指令后，TOUCHOUT才会变成低电平
    * @param    None
    * @return None
    */
void FPM383C_Sleep(void)
{
    FPM383C_SendData(12,PS_SleepBuffer);
}

/**
    * @brief    发送获取指纹图像指令
    * @param    Timeout：接收数据的超时时间
    * @return 确认码
    */
uint8_t FPM383C_GetImage(uint32_t Timeout)
{
    uint8_t Temp;
    FPM383C_SendData(12,PS_GetImageBuffer);
    while(USART2_STA<12 && (--Timeout))
    {
        rt_thread_mdelay(1);
    }
    Temp = (USART2_ReceiveBuffer[6] == 0x07 ? USART2_ReceiveBuffer[9] : 0xFF);
    memset(USART2_ReceiveBuffer,0xFF,sizeof(USART2_ReceiveBuffer));
    return Temp;
}

/**
    * @brief    发送生成模板指令
    * @param    Timeout：接收数据的超时时间
    * @return 确认码
    */
uint8_t FPM383C_GenChar1(uint32_t Timeout)
{
    uint8_t Temp;
    FPM383C_SendData(13,PS_GetChar1Buffer);
    while(USART2_STA<12 && (--Timeout))
    {
        rt_thread_mdelay(1);
    }
    Temp = (USART2_ReceiveBuffer[6] == 0x07 ? USART2_ReceiveBuffer[9] : 0xFF);
    memset(USART2_ReceiveBuffer,0xFF,sizeof(USART2_ReceiveBuffer));
    return Temp;
}


/**
    * @brief    发送搜索指纹指令
    * @param    Timeout：接收数据的超时时间
    * @return 确认码
    */
uint8_t FPM383C_SearchMB(uint32_t Timeout)
{
    FPM383C_SendData(17,PS_SearchMBBuffer);
    while(USART2_STA<16 && (--Timeout))
    {
        rt_thread_mdelay(1);
    }
    return (USART2_ReceiveBuffer[6] == 0x07 ? USART2_ReceiveBuffer[9] : 0xFF);
}

/**
    * @brief    删除指定指纹指令
  * @param  PageID ：需要删除的指纹ID号
    * @param    Timeout：接收数据的超时时间
    * @return 确认码
    */
uint8_t FPM383C_Delete(uint32_t TimeOut)
{
    rt_uint8_t ret = RT_EOK;

    key_buffer_pos_flag_clear();

    while(1){
        LOG_D("please input user password");
        ret = wait_for_confirm_key_press(TimeOut);

        if(RT_EOK != ret) return ret;

        /*check user password*/
        ret = password_compare(KeyBuffer, MyPassword.UserPassWord, MyPassword.Number_Of_UserPassWord, &KeyBuffer_Pos);
        if(RT_ERROR == ret){
            LOG_D("user password error in unlock password set checking");
            continue;
        }
        /*user password correct*/
        break;
    }

    while(1){
        LOG_D("please input a will delete fingerprint id");

        ret = wait_for_confirm_key_press(TimeOut);

        if(RT_EOK != ret) return ret;

        ret = fingerprint_id_check(KeyBuffer, &PageID, &KeyBuffer_Pos);
        key_buffer_pos_flag_clear();/* reset keybuffer and keybuffer_pos */
        if(RT_EBUSY != ret){

            continue;

        }
        PS_DeleteBuffer[10] = (PageID>>8);
        PS_DeleteBuffer[11] = (PageID);
        PS_DeleteBuffer[14] = (0x15+PS_DeleteBuffer[10]+PS_DeleteBuffer[11])>>8;
        PS_DeleteBuffer[15] = (0x15+PS_DeleteBuffer[10]+PS_DeleteBuffer[11]);
        FPM383C_SendData(16,PS_DeleteBuffer);
        while(USART2_STA<12 && (--TimeOut))
        {
          rt_thread_mdelay(1);
        }

        ret = (USART2_ReceiveBuffer[6] == 0x07 ? USART2_ReceiveBuffer[9] : 0xFF);
        if(0x00 != ret){
            LOG_D("deleted fail");
            continue;
        }
        LOG_D("deleted successful");
        fingerprint_id_set_or_delete(PageID, FINGERPRINT_IDDELETE_MODE);
        memset(USART2_ReceiveBuffer,0xFF,sizeof(USART2_ReceiveBuffer));
        return ret;

    }

}

/**
    * @brief    清空指纹库
    * @param    Timeout：接收数据的超时时间
    * @return 确认码
    */
uint8_t FPM383C_Empty(uint32_t TimeOut)
{
    rt_uint8_t ret;

    key_buffer_pos_flag_clear();

    while(1){
        LOG_D("please input user password");
        ret = wait_for_confirm_key_press(TimeOut);

        if(RT_EOK != ret) return ret;

        /*check user password*/
        ret = password_compare(KeyBuffer, MyPassword.UserPassWord, MyPassword.Number_Of_UserPassWord, &KeyBuffer_Pos);
        if(RT_ERROR == ret){
            LOG_D("user password error in unlock password set checking");
            continue;
        }
        /*user password correct*/
        break;
    }

    FPM383C_SendData(12,PS_EmptyBuffer);
    while(USART2_STA<12 && (--TimeOut))
    {
        rt_thread_mdelay(1);
    }

    ret = (USART2_ReceiveBuffer[6] == 0x07 ? USART2_ReceiveBuffer[9] : 0xFF);
    if(0x00 == ret){
        fingerprint_id_set_or_delete(PageID, FINGERPRINT_IDEMPTY_MODE);
        LOG_D("empty successful");
        return ret;
    }
    LOG_D("empty fail");
    memset(USART2_ReceiveBuffer,0xFF,sizeof(USART2_ReceiveBuffer));
    return ret;
}


/**
    * @brief    发送控制灯光指令
    * @param    PS_ControlLEDBuffer[]：不同颜色的协议数据
  * @param  Timeout：接收数据的超时时间
    * @return 确认码
    */
rt_uint8_t FPM383C_ControlLED(rt_uint8_t PS_ControlLEDBuffer[],rt_uint16_t Timeout)
{
    rt_uint8_t Temp;
    FPM383C_SendData(16,PS_ControlLEDBuffer);
    while(USART2_STA<12 && (--Timeout))
    {
        rt_thread_mdelay(1);
    }
    Temp = (USART2_ReceiveBuffer[6] == 0x07 ? USART2_ReceiveBuffer[9] : 0xFF);
    memset(USART2_ReceiveBuffer,0xFF,sizeof(USART2_ReceiveBuffer));
    return Temp;
}


/**
    * @brief    验证指纹流程
    * @param    None
    * @return 确认码
    */
void FPM383C_Identify(void)
{
    if(FPM383C_GetImage(2000) == 0x00)
    {
        if(FPM383C_GenChar1(2000) == 0x00)
        {
            if(FPM383C_SearchMB(2000) == 0x00)
            {
                MatchScore = (int)((USART2_ReceiveBuffer[10] << 8) + USART2_ReceiveBuffer[11]);
                LOG_D("USART2_ReceiveBuffer[10]:0x%x,USART2_ReceiveBuffer[11]:0x%x",USART2_ReceiveBuffer[10],USART2_ReceiveBuffer[11]);
                LOG_D("find finger ID:%d",(int)MatchScore);
                FPM383C_ControlLED(PS_GreenLEDBuffer,2000);
                return;
            }
        }
    }
    LOG_D("not found finger print");
    FPM383C_ControlLED(PS_RedLEDBuffer,2000);
}




/**
    * @brief    注册指纹函数
    * @param    PageID：输入需要注册的指纹ID号，取值范围0—59
    * @param    Timeout：设置注册指纹超时时间，因为需要按压四次手指，建议大于10000（即10s）
    * @return 确认码
    */
void FPM383C_Enroll(uint16_t TimeOut)
{
    rt_uint8_t ret = RT_EOK;
    key_buffer_pos_flag_clear();

    while(1){
        LOG_D("please input user password");
        ret = wait_for_confirm_key_press(TimeOut);

        if(RT_EOK != ret) return;

        /*check user password*/
        ret = password_compare(KeyBuffer, MyPassword.UserPassWord, MyPassword.Number_Of_UserPassWord, &KeyBuffer_Pos);
        if(RT_ERROR == ret){
            LOG_D("user password error in unlock password set checking");
            continue;
        }
        /*user password correct*/
        break;
    }

    while(1){

        LOG_D("please input new fingerprint id");
        ret = wait_for_confirm_key_press(TimeOut);

        if(RT_EOK != ret) return;

        ret = fingerprint_id_check(KeyBuffer, &PageID, &KeyBuffer_Pos);
        key_buffer_pos_flag_clear();/* reset keybuffer and keybuffer_pos */
        if(RT_EOK != ret){

            continue;
        }
        rt_thread_mdelay(5);
        PS_AutoEnrollBuffer[10] = (PageID>>8);
        PS_AutoEnrollBuffer[11] = (PageID);
        LOG_D("PS_AutoEnrollBuffer[10]:0x%x,PS_AutoEnrollBuffer[11]:0x%x",PS_AutoEnrollBuffer[10],PS_AutoEnrollBuffer[11]);
        PS_AutoEnrollBuffer[15] = (0x54+PS_AutoEnrollBuffer[10]+PS_AutoEnrollBuffer[11])>>8;
        PS_AutoEnrollBuffer[16] = (0x54+PS_AutoEnrollBuffer[10]+PS_AutoEnrollBuffer[11]);
        FPM383C_SendData(17,PS_AutoEnrollBuffer);

        LOG_D(">>>pleas press 4 times, each time more than 500ms<<<");
        while(USART2_STA<12 && (--TimeOut))
        {
          rt_thread_mdelay(1);
        }

        LOG_D("USART2_ReceiveBuffer[9]:0x%x",USART2_ReceiveBuffer[9]);
        if(USART2_ReceiveBuffer[9] == 0x00)
        {
            LOG_D(">>>finger print enroll successful<<<");
            fingerprint_id_set_or_delete(PageID, FINGERPRINT_IDSET_MODE);
            FPM383C_ControlLED(PS_GreenLEDBuffer,2000);
            return;
        } else if(TimeOut == 0){

            FPM383C_SendData(12,PS_CancelBuffer);

            rt_thread_mdelay(50);
        }
        LOG_D(">>>finger print enroll fail maybe its already exist<<<");
        FPM383C_ControlLED(PS_RedLEDBuffer,2000);
        return;
    }

}

void normal_state_fingerprint_handle(){
    if(0x80 == ScanStatus){

        FPM383C_ControlLED(PS_BlueLEDBuffer,2000);

        rt_thread_mdelay(5);

        FPM383C_Identify();                     //验证指纹模式

        rt_thread_mdelay(500);

        FPM383C_Sleep();

        ScanStatus = 0;

    }

}

void fingeprint_set_state_handle(rt_uint32_t TimeOut){
    rt_uint8_t flag = 1;
    while(TimeOut){
        if(flag){
            LOG_D("'A'enroll new finger print,'B'delete one finger print,'C'delete all finger print,'D'back to normal state");
            flag = 0;
        }
        switch(ConfirmKeyCode){
            case 0xFFF7://'A'
                if(FingerPrint_Number >= 60){
                    LOG_D("fingerprints number more than 60!");
                    ConfirmKeyCode = 0xFFFF;
                    flag = 1;
                    break;
                }
                TimeOut = DEFAULT_TIMEOUT;
                flag = 1;
                FPM383C_ControlLED(PS_BlueLEDBuffer,2000);
                FPM383C_Enroll(TimeOut);
                rt_thread_mdelay(500);
                FPM383C_Sleep();
                key_buffer_pos_flag_clear();
                break;

            case 0xFF7F://'B'
                LOG_D("FingerPrint_Number:%d",FingerPrint_Number);
                if(FingerPrint_Number <= 0){
                    LOG_D("not have any fingerprint");
                    ConfirmKeyCode = 0xFFFF;
                    flag = 1;
                    break;
                }
                TimeOut = DEFAULT_TIMEOUT;
                flag = 1;
                FPM383C_Delete(TimeOut);
                FPM383C_Sleep();
                key_buffer_pos_flag_clear();
                break;

            case 0xF7FF://'C'
                TimeOut = DEFAULT_TIMEOUT;
                flag = 1;
                FPM383C_Empty(TimeOut);
                FPM383C_Sleep();
                key_buffer_pos_flag_clear();
                break;
            case 0x7FFF:
                Current_SystemState = NORMAL_STATE;
                key_buffer_pos_flag_clear();
                LOG_D("fingerprint set state back to normal stae cause press 'D'");
                return;


        }


        --TimeOut;
        rt_thread_mdelay(1);
    }
    Current_SystemState = NORMAL_STATE;
    LOG_D("fingerprint set state back to normal stae cause Configure TimeOut");

}

ALIGN(RT_ALIGN_SIZE)
static char Thread_fpm383_Stack[2049];
static struct rt_thread Thread_Fpm383;

void thread_fpm383_entry(){

    while(1){
        switch(Current_SystemState){
            case NORMAL_STATE:
                normal_state_fingerprint_handle();
                break;
            case FINGERPRINT_SET_STATE:
                fingeprint_set_state_handle((rt_uint32_t)DEFAULT_TIMEOUT);
                break;
            default:
                break;
        }
        rt_thread_mdelay(50);
    }
}
int thread_fpm383_init(){

    fpm383c_uart_and_touchPin_Init();
        rt_thread_init(&Thread_Fpm383,
                       "td_Fpm383",
                       thread_fpm383_entry,
                       RT_NULL,
                       &Thread_fpm383_Stack,
                       sizeof(Thread_fpm383_Stack),
                       THREAD_FPM383_PRIORITY, THREAD_KFPM383_TIMESLICE);
        rt_thread_startup(&Thread_Fpm383);
        return 0;
}



