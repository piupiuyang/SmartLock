/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2023-03-21     piupiuY       the first version
 */

#include "ttp229.h"
#define DBG_TAG "ttp229"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

#define THREAD_KEYHANDLE_PRIORITY 20
#define THREAD_KEYHANDLE_TIMESLICE 100

#define KEY_PREES_ONESHOT 1
#define KEY_PREES_MORETHAN200 2
#define KEY_PREES_ERROR 3



static rt_timer_t Key_timer, Configure_timer;


static int timer_flag = 0;


PassWordGroup MyPassword ;

//按键按下的次数
rt_uint16_t KeyAppearCount;

//上一次按下的按键编码
rt_uint16_t PreKeyCode;

//当前按下的按键码
rt_uint16_t CurrentKeyCode;

//上一次按下的按键名
char PreKeyName;

//当前按下的按键名
char CurrentKeyName;

//由定时器确认的按键码
rt_uint16_t ConfirmKeyCode;

//由定时器确认的按键名
char ConfirmKeyName;

//当前系统状态
rt_uint8_t Current_SystemState;

//按键下标位置
rt_uint8_t KeyBuffer_Pos = 0;

//判断按键缓冲区是否填满过
rt_uint8_t isKeyBuffFull_Flag = 0;

//按键缓冲区
char KeyBuffer[13] = {0};




uint16_t ttp229_Read(void){

    rt_uint16_t DATA = 0 ;

        for( rt_uint8_t i = 0; i < KEY_MAX; i++){

            rt_pin_write(SCL_PIN, 1);

            rt_hw_us_delay(100);

            rt_pin_write(SCL_PIN, 0);

            DATA |= rt_pin_read(SDO_PIN) << i;

            rt_hw_us_delay(100);

          }
    rt_thread_mdelay(2);

    return DATA & 0xFFFF;

}

int get_keypress_state(){
    if((KeyAppearCount < 140) && (KeyAppearCount > 0)){
            KeyAppearCount = 0;
            return KEY_PREES_ONESHOT;

        }else if(KeyAppearCount == 0){

            return KEY_PREES_ERROR;

        }else {
            KeyAppearCount = 0;
            return KEY_PREES_MORETHAN200;

        }

    return 0;
}

int ttp29_getkey(void){
    rt_uint8_t ret;
    CurrentKeyCode = ttp229_Read();
    LOG_D("CurrentKeyCode:0x%x",CurrentKeyCode);
    singelkey_get(CurrentKeyCode);
    if(0xFFFF == CurrentKeyCode){
        ret = get_keypress_state();
        switch(ret){
        case KEY_PREES_ONESHOT:
            LOG_D("CurrentKey just press oneshot in getkey");
            if(KeyBuffer_Pos % 13 == 0){
                KeyBuffer_Pos = 0;
                isKeyBuffFull_Flag ++;
                LOG_D("isKeyBuffFull_Flag: %d",isKeyBuffFull_Flag);
            }
            KeyBuffer[KeyBuffer_Pos] = PreKeyName;
            ++KeyBuffer_Pos;
            LOG_D("KeyBuffer:%s",KeyBuffer);
            ConfirmKeyName = PreKeyName;
            ConfirmKeyCode = PreKeyCode;
            break;
        case KEY_PREES_ERROR:
            break;
        case KEY_PREES_MORETHAN200:
            LOG_D("in KEY_PREES_MORETHAN200 PreKeyCode:0x%x",PreKeyCode);
            switch(PreKeyCode){
            //按键A设置密码
            case 0xFFF7:
                if(NORMAL_STATE != Current_SystemState)
                    break;
                Current_SystemState = PASSWORD_SET_STATE;
                ConfirmKeyCode = 0xFFFF;
                break;

            //按键B设置指纹
            case 0xFF7F:
                if(NORMAL_STATE != Current_SystemState)
                    break;
                Current_SystemState = FINGERPRINT_SET_STATE;
                ConfirmKeyCode = 0xFFFF;
                break;

            //按键C设置人脸
            case 0xF7FF:
                if(NORMAL_STATE != Current_SystemState)
                    break;
                Current_SystemState = FACE_SET_STATE;
                ConfirmKeyCode = 0xFFFF;
                break;

            }
            break;
        }
        BEEP_OFF;
        rt_timer_stop(Key_timer);
        timer_flag = 0;
        return 1;
    }
    ssd1306_SetCursor(0, 20);
    ssd1306_WriteChar(CurrentKeyName, Font_7x10, White);
    ssd1306_UpdateScreen();
    if(CurrentKeyCode == PreKeyCode){
        ++KeyAppearCount;
        BEEP_ON;

    }else {
        BEEP_OFF;
        KeyAppearCount = 0;
    }
    PreKeyCode = CurrentKeyCode;
    PreKeyName = CurrentKeyName;
    LOG_D("KEY_NAME:%c.,AppearCount:%d",CurrentKeyName,KeyAppearCount);

    return 0;
}

static void key_timeout(){
    rt_uint8_t ret;

    ret = get_keypress_state();
    if(KEY_PREES_ONESHOT == ret){

        LOG_D("CurrentKey just press oneshot in key timeout");
        if(KeyBuffer_Pos % 13 == 0){
            KeyBuffer_Pos = 0;
            isKeyBuffFull_Flag = 1;
        }
        KeyBuffer[KeyBuffer_Pos] = CurrentKeyName;
        ++KeyBuffer_Pos;
        LOG_D("KeyBuffer:%s",KeyBuffer);
        ConfirmKeyName = CurrentKeyName;
        ConfirmKeyCode = CurrentKeyCode;

    }else if(KEY_PREES_MORETHAN200 == ret){

        LOG_D("In_key_timeout CurrentKey press time  more than 200ms");


    }
    BEEP_OFF;
    rt_timer_stop(Key_timer);
    timer_flag = 0;

}

static void cfg_timeout(){


}

void singelkey_get(rt_uint16_t KeyCode){

    switch(KeyCode){

        case 0xFFFE: CurrentKeyName = '1';return ;
        case 0xFFFD: CurrentKeyName = '2';return ;
        case 0xFFFB: CurrentKeyName = '3';return ;
        case 0xFFEF: CurrentKeyName = '4';return ;
        case 0xFFDF: CurrentKeyName = '5';return ;
        case 0xFFBF: CurrentKeyName = '6';return ;
        case 0xFEFF: CurrentKeyName = '7';return ;
        case 0xFDFF: CurrentKeyName = '8';return ;
        case 0xFBFF: CurrentKeyName = '9';return ;
        case 0xEFFF: CurrentKeyName = '*';return ;
        case 0xDFFF: CurrentKeyName = '0';return ;
        case 0xBFFF: CurrentKeyName = '#';return ;
        case 0x7FFF: CurrentKeyName = 'D';return ;
        case 0xF7FF: CurrentKeyName = 'C';return ;
        case 0xFF7F: CurrentKeyName = 'B';return ;
        case 0xFFF7: CurrentKeyName = 'A';return ;

    }

    return ;

}



void ttp229_init(){
    rt_pin_mode(SCL_PIN, PIN_MODE_OUTPUT);
    rt_pin_mode(SDO_PIN, PIN_MODE_INPUT_PULLUP);
    password_set();
    rt_pin_attach_irq(SDO_PIN, PIN_IRQ_MODE_RISING, (void *)ttp29_getkey, RT_NULL);
    rt_pin_irq_enable(SDO_PIN, PIN_IRQ_ENABLE);

    //beep_init(BEEP_PIN, PIN_LOW);
    rt_pin_mode(BEEP_PIN,PIN_MODE_OUTPUT);  //蜂鸣器 IO初始化
    Key_timer  = rt_timer_create("Key_timer", key_timeout,  RT_NULL, 75, RT_TIMER_FLAG_ONE_SHOT);
    Configure_timer = rt_timer_create("Cfg_timer", cfg_timeout, RT_NULL, 20000, RT_TIMER_FLAG_ONE_SHOT);

}

void normal_state_keyhandle(){
int ret = 1;
    if(0xBFFF == ConfirmKeyCode){
        ret = password_compare(KeyBuffer, MyPassword.PassWord, MyPassword.Number_Of_PassWord, &KeyBuffer_Pos);
        if(ret == 0){

            LOG_D("password correct");

        }else {

            LOG_D("password error");

        }

        ConfirmKeyCode = 0xFFFF;
    }

}

void key_buffer_pos_flag_clear(){
    KeyBuffer_Pos = 0;
    isKeyBuffFull_Flag = 0;
    memset(KeyBuffer, 0 ,sizeof(KeyBuffer));/* reset keybuffer and keybuffer_pos */
}

rt_uint8_t wait_for_confirm_key_press(rt_uint32_t TimeOut){

    while((0xBFFF != ConfirmKeyCode) && TimeOut){
        --TimeOut;
        if(0x7FFF == ConfirmKeyCode){
            //press key 'D', back to normal state
            ConfirmKeyCode = 0xFFFF;
            LOG_D("back to last stae cause press 'D'");
            return RT_EINTR;
        }
        rt_thread_mdelay(1);
    }
    if(TimeOut == 0){ //TimeOut back to normal state
        LOG_D("back to last stae cause Configure TimeOut");
        return RT_ETIMEOUT;
    }
    ConfirmKeyCode = 0xFFFF;
    return RT_EOK;

}

void unlock_or_user_password_reset(rt_uint8_t Mode,rt_uint32_t TimeOut){
    rt_uint8_t ret = RT_EOK;
    rt_uint8_t Key_BufferPos_Mask = 0;
    char TempKeyBuffer[13];

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
       LOG_D("please input new password");

       ret = wait_for_confirm_key_press(TimeOut);

       if(RT_EOK != ret) return;

       ret = password_check(KeyBuffer, &KeyBuffer_Pos);//Check if the password is legitimate
       if(RT_ERROR == ret){
           LOG_D("password illegal");
           continue;
       }
       strcpy(TempKeyBuffer,KeyBuffer);
       Key_BufferPos_Mask = KeyBuffer_Pos;
       key_buffer_pos_flag_clear();/* reset keybuffer and keybuffer_pos */
       LOG_D("please input new password again");

       ret = wait_for_confirm_key_press(TimeOut);

       if(RT_EOK != ret) return;

       ret = password_compare(KeyBuffer, TempKeyBuffer, Key_BufferPos_Mask - 1, &KeyBuffer_Pos);//compare twice password
       if(RT_ERROR == ret){
           LOG_D("The two passwords are inconsistent ");
           continue;
       }
       if(CHANGE_UNLOCKPASSWORD_MODE == Mode){

           ret = password_reset(TempKeyBuffer, MyPassword.PassWord, &Key_BufferPos_Mask, Mode);//重置开锁密码

       }else{
           ret = password_reset(TempKeyBuffer, MyPassword.UserPassWord, &Key_BufferPos_Mask, Mode);//重置用户密码
       }

       if(RT_EOK != ret){
           LOG_D("passwords reset error in  password_reset");
           continue;
       }

       Current_SystemState = NORMAL_STATE;
       break;

    }

}

void password_set_state_keyhandle(rt_uint32_t TimeOut){

    rt_uint8_t flag = 1;
    while(TimeOut){
        if(flag){
            LOG_D("'A'reset unlcok password,'B'reset user password,'D'back to  normal state");
            flag = 0;
        }
        switch(ConfirmKeyCode){

        case 0xFFF7:// 'A'
            TimeOut = DEFAULT_TIMEOUT;
            flag = 1;
            unlock_or_user_password_reset(CHANGE_UNLOCKPASSWORD_MODE, TimeOut);
            key_buffer_pos_flag_clear();
            break;
        case 0xFF7F://'B'
            TimeOut = DEFAULT_TIMEOUT;
            flag = 1;
            unlock_or_user_password_reset(CHANGE_USERPASSWORD_MODE, TimeOut);
            key_buffer_pos_flag_clear();
            break;
        case 0x7FFF://'D'
            Current_SystemState = NORMAL_STATE;
            key_buffer_pos_flag_clear();
            LOG_D("password set state back to normal stae cause press 'D'");
            return;
        }
        --TimeOut;
        rt_thread_mdelay(1);
    }
    Current_SystemState = NORMAL_STATE;
    LOG_D("password set state back to normal stae cause Configure TimeOut");

}

ALIGN(RT_ALIGN_SIZE)
static char Thread_keyhandle_Stack[2049];
static struct rt_thread Thread_KeyHandle;

void thread_keyhandle_entry(){

    while(1){
        switch(Current_SystemState){
            case NORMAL_STATE:
                normal_state_keyhandle();
                break;
            case PASSWORD_SET_STATE:
                password_set_state_keyhandle((rt_uint32_t)DEFAULT_TIMEOUT);
                break;
            default:
                break;
        }
        rt_thread_mdelay(50);
    }

}

int thread_keyhandle_init(){

    ttp229_init();

    rt_thread_init(&Thread_KeyHandle,
                   "td_keyhdl",
                   thread_keyhandle_entry,
                   RT_NULL,
                   &Thread_keyhandle_Stack,
                   sizeof(Thread_keyhandle_Stack),
                   THREAD_KEYHANDLE_PRIORITY, THREAD_KEYHANDLE_TIMESLICE);
    rt_thread_startup(&Thread_KeyHandle);

    return 0;

}
