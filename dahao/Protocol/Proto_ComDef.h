/*******************************************************************************************************************
|  File Name: ProtoComDef.h
|
|  Description:
|
|-----------------------------------------------------------------------------
|               C O P Y R I G H T
|-----------------------------------------------------------------------------
| Copyright (c) 2002-2012 by DAHAO TECHNOLOGY Co., Ltd.All rights reserved.
|
|               This software is copyright protected and proprietary 
|               to DAHAO TECHNOLOGY Co., Ltd. DAHAO TECHNOLOGY Co., Ltd 
|               grants to you only those rights as set out in the 
|               license conditions. All other rights remain with 
|               DAHAO TECHNOLOGY Co., Ltd.
|-----------------------------------------------------------------------------
|               A U T H O R   I D E N T I T Y
|-----------------------------------------------------------------------------
| Initials     Name                      Company
| --------     ---------------------     ------------------------------------------
|                          DAHAO TECHNOLOGY Co., Ltd
|-----------------------------------------------------------------------------
|               R E V I S I O N   H I S T O R Y
|-----------------------------------------------------------------------------
| Date                 Ver     Author     Description
| ---------------------------- ------------------------------------------------
| 2012-10-23  1.00                  Initial version 
|******************************************************************************************************************/
#ifndef MOAPDEF_H
#define MOAPDEF_H

/******************************************************************************
**********************Macro definitiont*******************************************
******************************************************************************/
typedef enum 
{
	COMD_GET_HEARTBEAT = 0x01,
	COMD_SET_IP = 0x02,
	COMD_GET_IP = 0x03,
	COMD_SET_REST = 0x04,
	COMD_SET_TIME = 0x05,
	COMD_GET_TIME = 0x06,
	COMD_CONFIG_HOST = 0x07, /* ����ͨѶ���������Ĳ��� */
	COMD_READER_UNLOAD_CARD_ID = 0x08, /* ����Ƭ���ϴ�ID  */
	COMD_READER_WRITE = 0x09, /* д��*/
	COMD_READER_READ = 0x0A, /* ����*/
	COMD_SET_TIME_APP = 0x7D,
	COMD_GET_TIME_APP = 0x7C,

	COMD_GET_CARLOCK_FIFO = 0x0A,
	COMD_SET_CARLOCK_FIFO = 0x0b,

	CMD_O2O_READ_ID = 0x15,
	COMD_MODIFY_CONFIG = 0x16,  // �·�������Ϣ16
	COMD_READ_CONFIG,  // 17
	COMD_MODIFY_PWSD,   //�޸��豸����18
	COMD_MODIFY_DEFAULT_PASWD = 0x59, /* ����Ĭ������*/

	CMD_O2O_READ_OTHER_KEY = 0x22,  // ��ȡ������22
	CMD_O2O_ADD_OTHER_KEY,  // ��������/��Ƭ23
	CMD_O2O_DELETE_OTHER_KEY,  // ɾ������/��Ƭ24
     
	COMD_GET_ID_TOTAL = 0x25,
	COMD_GET_LIST = 0x26,
	COMD_SET_LIST = 0x27,
	COMD_REMOVE_ID = 0x28,
	COMD_LARNCARD_OK_ID = 0x2C,

	COMD_LOCK_UP = 0x1B,
	COMD_POWER_ON = COMD_LOCK_UP,
	COMD_LOCK_DOWN = 0x1C,
	COMD_LOAD_RECORD = 0x46, /* ��ȡ������¼*/
	COMD_REMOVE_RECORD = 0x47, /*�Ƴ���¼*/

	CMD_O2O_OPEN_DEVICE_EXT = 0x1E,  // ����ָ���IE

	CMD_ADD_OTHER_KEY,  // ��������/��Ƭ23 û�а�û�

	COMD_UPLOAD_RECORD = 0x40, /* �ϴ�������¼*/
	COMD_SET_PROPERTY_BUILD_ID = 0x42,  /* ����С����ź�¥�����*/
	COMD_GET_PROPERTY_BUILD_ID = 0x41,  /* ��ȡС����ź�¥�����*/
	COMD_GET_PUBLIC_BUILD_ID = 0x43,      /*  ��ȡ������¥�����*/
	COMD_SET_PUBLIC_BUILD_ID = 0x44,  /*  ���ù�����¥�����*/
	COMD_DELETE_PUBLIC_BUILD_ID = 0x45,  /*  ɾ��������¥�����*/
	
	COMD_SET_ID= 0x50,
	COMD_GET_ID= 0x51,

	COMD_BEEP= 0x52,
	COMD_GET_APN= 0x53,
	COMD_SET_APN= 0x54,

	COMD_ALARM= 0x55,
	COMD_ALWAYS_STATE= 0x56,
	COMD_SET_CONFIG_NEW = 0x5B,
	COMD_READ_CONFIG_NEW = 0x5C,
	COMD_SEND_WGID= 0x61,
	CMD_GET_SYS_INFO = 0x62, /* ��ȡ�豸ϵͳ��Ϣ ���������������豸��ȡSYS_ID, MAC, PASSWORD */
        COMD_SET_RF_PARA = 0x6D,  
	COMD_GET_RF_PARA = 0x6E, 
	COMD_MODIFY_CUSTOM_INFO = 0x70, /* ���Ŀͻ�����*/
	CMD_DEFAULT_DEVICE = 0x79, /* �ָ���������*/
	COMD_SET_PARA_DEF = CMD_DEFAULT_DEVICE,
	
	COMD_O2O_SET_TOUCH_SEN = 0x71,
	CMD_O2O_READ_VERSION = 0x7A,
	COMD_SET_DECRP_WORD = 0x7e,
	COMD_RESET_DEVICE = 0x7F,
	COMD_REQ_OPEN = 0x57,
	COMD_FACTORY_TEST = 0x85, /* ��������*/
	COMD_GET_DAT = 0xFE
}ProtoCommDef_Type;


typedef enum 
{
	COMD_LORA_SNYC=0,
	COMD_LORA_REG=0x01,
	COMD_LORA_OPEN = 0x02,
	COMD_LORA_CLOSE = 0x03,
	COMD_LORA_SETLIST=0x04,
	COMD_LORA_REMOVE=0x05,
	COMD_LORA_REST=0x06,
	COMD_LORA_SET_TIME= 0x07,
	COMD_LORA_CFGRF=0x08,
	COMD_LORA_RECORE=0x09,
	COMD_LORA_CONFIG=0x0a,
	COMD_LORA_ADDKEY=0x0b,
}ProtoLoraQuicCommDef_Type;


typedef enum
{
	COMD_FATORY_IDLE = 0,
	COMD_FATORY_ENTERN_SLEEP,
	COMD_FATORY_PARA_INIT,
	COMD_FATORY_MOTOR_TEST,
}FatoryTest_Type;

#define COMD_ERR_OK					0x00
#define COMD_ERR_NG				       0x01
#define COMD_ERR_BUSY					0x01
#define COMD_ERR_LOPOWER				0x02
#define COMD_ERR_CMD_US				0x03
#define COMD_ERR_SUBCMD_US			0x04
#define COMD_ERR_DATA					0x05
#define COMD_ERR_SEG_TYPE				0x06
#define COMD_ERR_SEG_PARA				0x07
#define COMD_ERR_SEG_ADDR				0x08
#define COMD_ERR_EP_ADDR				0x09
#define COMD_ERR_KEY					0x0A
#define COMD_ERR_MODEL					0x0B
#define COMD_ERR_UNKNOWN				0xFF


enum
{
	CONFIG_FLAG_PASWD = 0x01, 
	CONFIG_FLAG_SETTING = 0x02, 
	CONFIG_FLAG_NAME = 0x04, 
	CONFIG_FLAG_SLEEP_READ_CARD = 0x08, /* TRUE ����ģʽ�¶��� */
	CONFIG_FLAG_CHECK_TIME = 0x10,   /*���뿪���Ϳ�Ƭ������Ҫ�Ƚ�ʱ��*/
	CONFIG_FLAG_TOUCH_BEEP = 0x20,   /*������ʾ��*/
	CONFIG_FLAG_LOW_ALARM = 0x40,   /*�� ѹ����*/
	CONFIG_FLAG_TOUCH_ACTIVE = 0x80,  /* ������㲥*/
};

enum
{
	CONFIG_KEY_AUTOLOCK = 1, /* �Զ�����+ ����ʱ��*/
	CONFIG_KEY_BTSAVEMODE,  /* ����ʡ��ģʽ ������㲥:1*/
	CONFIG_KEY_DETCARDSLEEP, /* ��Ƭһֱ����:1*/
	CONFIG_KEY_DISIBLEDOORLOCK, /* �ر��Ŵ�:1*/
};
/******************************************************************************
**********************Variable definition***************************************
******************************************************************************/



/******************************************************************************
**********************Function declared****************************************
******************************************************************************/

#endif //_COLBUS_DEF_H_


