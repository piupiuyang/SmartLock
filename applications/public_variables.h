/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2023-04-17     piupiuY       the first version
 */
#ifndef APPLICATIONS_PUBLIC_VARIABLES_H_
#define APPLICATIONS_PUBLIC_VARIABLES_H_
#define CHANGE_USERPASSWORD_MODE 0
#define CHANGE_UNLOCKPASSWORD_MODE 1
#define DEFAULT_TIMEOUT 15000
#define NORMAL_STATE 0
#define CONFIGURE_STATE 1
#define PASSWORD_SET_STATE 2
#define FINGERPRINT_SET_STATE 3
#define FACE_SET_STATE 4

typedef struct PassWordGroup{

    //密码有几位数 默认为6位
    rt_uint8_t Number_Of_PassWord ;

    //密码数组
    char PassWord[12] ;

    //用户密码有几位数 默认为6位
    rt_uint8_t Number_Of_UserPassWord ;

    //用户密码数组
    char UserPassWord[12] ;

}PassWordGroup;
extern PassWordGroup MyPassword ;
extern rt_uint16_t KeyAppearCount;
extern char ConfirmKeyName;
extern rt_uint8_t Current_SystemState;
extern rt_uint8_t KeyBuffer_Pos;
extern char KeyBuffer[13];
extern rt_uint16_t ConfirmKeyCode;
extern rt_uint8_t isKeyBuffFull_Flag;
extern rt_sem_t fingerprint_set_state_over_sem;
extern rt_uint8_t FingerPrint_Number;

int password_set();
int password_compare(char src_buffer[13], char* dest_buffer,rt_uint8_t num_of_password,rt_uint8_t* key_buffer_pos);
int password_reset(char src_buffer[13], char dest_buffer[12],rt_uint8_t* key_buffer_pos,rt_uint8_t Mode);
int user_password_rest(char src_buffer[13], char dest_buffer[12],rt_uint8_t num_of_password,rt_uint8_t* key_buffer_pos);
int password_check(char src_buffer[13],rt_uint8_t* key_buffer_pos);
int fingerprint_id_check(char src_buffer[13], rt_uint8_t* PageID, rt_uint8_t* key_buffer_pos);
void key_buffer_pos_flag_clear();
void fingerprint_id_set_or_delete(rt_uint8_t PageID , rt_uint8_t Mode);

rt_uint8_t wait_for_confirm_key_press(rt_uint32_t TimeOut);
#endif /* APPLICATIONS_PUBLIC_VARIABLES_H_ */
