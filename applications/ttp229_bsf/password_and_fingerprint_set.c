/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2023-04-16     piupiuY       the first version
 */
#include "at24cxx.h"
#include "public_variables.h"
#define DBG_TAG "password_set"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>
#include <string.h>
//#include "ttp229.h"
#define AT24C02_I2C_BUS_NAME     "i2c1"
#define FIRSTPAGE 0
#define NUMBER_OF_PASSWORD_POS 0x02
#define NUMBER_OF_USERPASSWORD_POS 0x01
#define NUMBER_OF_FINGERPRINT_AND_START_POS 0x1A
#define USERPASSWORD_POS 0x03
#define PASSWORD_POS 0x0F

#define PAGESIZE 8
#define PASSWORD_BYTESIZE
static at24cxx_device_t dev = RT_NULL;

rt_uint8_t Default_Number_Of_PassWord = 6;
rt_uint8_t Default_Number_Of_FingerPrint = 0;
rt_uint8_t Default_Value_Of_FingerPrint[60] = {0x00};
char Default_PassWord[12] = {'1','2','3','4','5','6'};
char Default_UserPassWord[12] = {'6','6','6','6','6','6'};
//char PassWord_Init_State = '0';
int password_set(){
    rt_uint8_t ret;
    dev = at24cxx_init(AT24C02_I2C_BUS_NAME,FIRSTPAGE);
    ret = at24cxx_read_one_byte(dev, FIRSTPAGE);
    if(0x80 != ret){
        at24cxx_write_one_byte(dev, FIRSTPAGE, 0x80);
        rt_thread_mdelay(10);
        at24cxx_write_one_byte(dev, NUMBER_OF_USERPASSWORD_POS, Default_Number_Of_PassWord);
        rt_thread_mdelay(10);
        at24cxx_write_one_byte(dev, NUMBER_OF_PASSWORD_POS, Default_Number_Of_PassWord);
        rt_thread_mdelay(10);
        at24cxx_write_one_byte(dev, NUMBER_OF_FINGERPRINT_AND_START_POS, Default_Number_Of_FingerPrint);
        rt_thread_mdelay(10);
        at24cxx_write(dev, USERPASSWORD_POS, (rt_uint8_t *)Default_UserPassWord, Default_Number_Of_PassWord);
        rt_thread_mdelay(10);
        at24cxx_write(dev, PASSWORD_POS, (rt_uint8_t *)Default_PassWord, Default_Number_Of_PassWord);
        rt_thread_mdelay(10);
        at24cxx_write(dev, NUMBER_OF_FINGERPRINT_AND_START_POS + 1, Default_Value_Of_FingerPrint, 60);
        rt_thread_mdelay(10);
    }
    MyPassword.Number_Of_UserPassWord = at24cxx_read_one_byte(dev, NUMBER_OF_USERPASSWORD_POS);
    MyPassword.Number_Of_PassWord = at24cxx_read_one_byte(dev, NUMBER_OF_PASSWORD_POS);
    FingerPrint_Number = at24cxx_read_one_byte(dev, NUMBER_OF_FINGERPRINT_AND_START_POS);
    ret = at24cxx_read(dev, USERPASSWORD_POS, (rt_uint8_t*)MyPassword.UserPassWord, MyPassword.Number_Of_UserPassWord);
    if(ret != RT_EOK){
        LOG_D("UserPassword read error in password_set ");
    }
    ret = at24cxx_read(dev, PASSWORD_POS, (rt_uint8_t*)MyPassword.PassWord, MyPassword.Number_Of_PassWord );
    if(ret != RT_EOK){
        LOG_D("UserPassword read error in password_set");
    }
    return 0;
}

int password_compare(char src_buffer[13], char dest_buffer[12],rt_uint8_t num_of_password, rt_uint8_t* key_buffer_pos){
    rt_uint8_t ret = RT_EOK;
    LOG_D("*key_buffer_pos:%d,num_of_password:%d,isKeyBuffFull_Flag:%d",*key_buffer_pos,num_of_password,isKeyBuffFull_Flag);
    if((((*key_buffer_pos)-1) > num_of_password)||(isKeyBuffFull_Flag > 1)){
        key_buffer_pos_flag_clear();
        LOG_D("num_of_password error in password_compare");
        return RT_ERROR;
    }

    for(rt_uint8_t i = 0 ; i < num_of_password ; i++){
               LOG_D("KeyBuffer[i]:%c,PassWord[i]:%c",src_buffer[i],dest_buffer[i]);
               if(src_buffer[i] == dest_buffer[i]){
                   continue;
               }else {
                   ret = 1;
                   break;
               }

           }

    key_buffer_pos_flag_clear();
    if(RT_EOK == ret){

        return RT_EOK;

    }

    return RT_ERROR;

}

int password_check(char src_buffer[13],rt_uint8_t* key_buffer_pos){
    if((*key_buffer_pos < 6) || (isKeyBuffFull_Flag > 1) ){
        key_buffer_pos_flag_clear();
        LOG_D("num_of_password error in password_check");
        return RT_ERROR;
    }

    return RT_EOK;

}

int fingerprint_id_check(char src_buffer[13], rt_uint8_t *PageID, rt_uint8_t* key_buffer_pos){

    rt_uint8_t ret;


    if((*key_buffer_pos > 3) || (isKeyBuffFull_Flag > 1) ){
        key_buffer_pos_flag_clear();
        LOG_D("num_of_fpid error in fingerprint_id_check");
        return RT_ERROR;
    }
    *PageID = atoi(src_buffer);

    LOG_D("PageID:%d",*PageID);
    if((*PageID > 60)||(*PageID < 1)){
        LOG_D("id illegal");
        return RT_ERROR;
    }

    ret = at24cxx_read_one_byte(dev, (NUMBER_OF_FINGERPRINT_AND_START_POS + (*PageID)));
    LOG_D("ret:0x%x",ret);
    if((ret & 0x80) == 0x80){
        LOG_D("id already exist");
        return RT_EBUSY;
    }

    LOG_D("id not exist");

    return RT_EOK;

}

void fingerprint_id_set_or_delete(rt_uint8_t PageID , rt_uint8_t Mode){

    rt_uint8_t ret ;

    if(0 == Mode){
        at24cxx_write_one_byte(dev, NUMBER_OF_FINGERPRINT_AND_START_POS, FingerPrint_Number+1);
        rt_thread_mdelay(10);
        FingerPrint_Number += 1;
        at24cxx_write_one_byte(dev, (NUMBER_OF_FINGERPRINT_AND_START_POS + PageID), 0x80);
        rt_thread_mdelay(10);
        ret = at24cxx_read_one_byte(dev, NUMBER_OF_FINGERPRINT_AND_START_POS + PageID);
        LOG_D("ret:0x%x in fingerprint_id_set_or_delete",ret);
    }else if(1 == Mode){
        at24cxx_write_one_byte(dev, NUMBER_OF_FINGERPRINT_AND_START_POS, FingerPrint_Number-1);
        FingerPrint_Number -= 1;
        rt_thread_mdelay(10);
        at24cxx_write_one_byte(dev, NUMBER_OF_FINGERPRINT_AND_START_POS + PageID, 0x00);
    }else if(2 == Mode){
        at24cxx_write_one_byte(dev, NUMBER_OF_FINGERPRINT_AND_START_POS, Default_Number_Of_FingerPrint);
        FingerPrint_Number = Default_Number_Of_FingerPrint;
        rt_thread_mdelay(10);
        at24cxx_write(dev, NUMBER_OF_FINGERPRINT_AND_START_POS + 1, Default_Value_Of_FingerPrint, 60);
    }


}

void clear_all_of_fingerprint(){



}

//Mode 1:reset unlock password,Mode 0:reset user password
int password_reset(char src_buffer[13], char dest_buffer[12],rt_uint8_t* key_buffer_pos,rt_uint8_t Mode){

    rt_uint8_t  ret = RT_EOK ;
    if((*key_buffer_pos-1 < 6) || (isKeyBuffFull_Flag > 1) ){
        key_buffer_pos_flag_clear();
        LOG_D("num_of_password error in password_reset");
        return RT_ERROR;

    }
    if(CHANGE_USERPASSWORD_MODE == Mode){
        MyPassword.Number_Of_UserPassWord = *key_buffer_pos - 1;
        ret = at24cxx_write_one_byte(dev, NUMBER_OF_USERPASSWORD_POS, *key_buffer_pos - 1);
    }else{
        MyPassword.Number_Of_PassWord = *key_buffer_pos - 1;
        ret = at24cxx_write_one_byte(dev, NUMBER_OF_PASSWORD_POS, *key_buffer_pos - 1);
    }
    if(ret){
        LOG_D("num_of_password wirte error in password_reset");
        return RT_EBUSY;
    }
    rt_thread_mdelay(7);

    if(CHANGE_USERPASSWORD_MODE == Mode){
        at24cxx_write(dev, USERPASSWORD_POS, (rt_uint8_t *)src_buffer, *key_buffer_pos - 1 );
    }else{
        at24cxx_write(dev, PASSWORD_POS, (rt_uint8_t *)src_buffer, *key_buffer_pos - 1 );
    }
    if(RT_EOK != ret){
        LOG_D("password wirte error in password_reset ret: %d",ret);
        return RT_EBUSY;
    }

    memccpy(dest_buffer,src_buffer,'#',12);

    LOG_D("password reset ok");
    key_buffer_pos_flag_clear();
    return RT_EOK;
}

