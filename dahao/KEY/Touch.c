/*****************************************************************************
|  File Name: Key.c
|
|  Description: 
|
|-----------------------------------------------------------------------------
|               C O P Y R I G H T
|-----------------------------------------------------------------------------
| Copyright (c) 2002-2012 by DAHAO TECHNOLOGY Co., Ltd. All rights reserved.
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
| --------     ---------------------     -------------------------------------
| LXW          LXW               DAHAO TECHNOLOGY Co., Ltd
| YJW          YJW                DAHAO TECHNOLOGY Co., Ltd
|-----------------------------------------------------------------------------
|               R E V I S I O N   H I S T O R Y
|-----------------------------------------------------------------------------
| Date        Ver     Author  Description
| ---------   ----    ------  ------------------------------------------------
| 2016-05   1.00    yjw     Initial version 
|****************************************************************************/

#define SJT8012_I2C_ADDRESS     0x13

/****************************************************************************/ 
/* Include file                                                             */                                                 
/****************************************************************************/
#include  "KEY\key.h"
#include  "SYSTEM\sys.h"
#include  "SYSTEM\Sys_GenFun.h"
#include  "I2C\I2c_Soft.h"
#include  "KEY\Touch.h"
#include  "Beep\Beep.h"
#include  "Access\Access.h"
#include  "protocol\Proto_Analy.h"
#include  "Access\LockKey.h"
#include  "Encpt\Encpt.h"
#include  "KEY\TSMxx.h"
#include  "HC595\HC595.h"
#include "Light\light.h"
#include "Encpt\Encpt.h"
#include "Flash\Flash.h"
//#include "Sensor\sensor.h"

/*static */uint8 TouchBuf[(TOUCH_KEY_BUF_MAX>>1)+1];


uint8 TouchState;
uint8 TouchKeyIndex;
uint8 TouchTimer64ms=0; 		// 按键超时
//uint8 TouchCfgTiems =0;
uint16 TouchRstTimes =0;

const uint16 TouchLightStable[]=
{
	// 0  1 2 3   4 5 6   7 8 9   * 0 #
	//~(0xF800),~(0xF004),~(0xF002),~(0xF001),~(0xF010),~(0xF008),~(0xF100),~(0xF040),~(0xF020),~(0xF200),~(0xF080),~(0xF400),~(0xF400)
	  ~(0x0001),~(0x0010),~(0x0008),~(0x0100),~(0x0020),~(0x0004),~(0x1000),~(0x0040),~(0x0002),~(0x0800),~(0x0080),~(0x0400),~(0x0400)
};
//HC595_LedWriteData(0x0010|0x0008|0x0100|0x0004|0x0001|0x0002);

void Touch_Clear(void);

/****************************************************************************************************
**Function:
   void Touch_Init( void )
**Author: rory
**Description:
**Input: 
**Output: 
****************************************************************************************************/
void Touch_Init( void )
{
	Tsmxx_Open();
	Touch_Clear();
	TouchTimer64ms = 0;
}
void Touch_Clear(void)
{
	TouchState = TOUCH_STATE_IDLE;
	TouchKeyIndex = 0;
	for(uint8 i=0; i<(TOUCH_KEY_BUF_MAX>>1); i++)
	{
		TouchBuf[i] = KEY_NULL;
	}
	//TouchCfgTiems = 0;
}
/****************************************************************************************************
**Function:
   void Touch_ResetProc( void )
**Author: rory
**Description:
**Input: 
**Output: 
****************************************************************************************************/
/*void Touch_ResetProc(void)
{
	if(TouchTimer64ms != 0)
	{
		return;
	}
#if (defined LORA_ENABLE) && (LORA_ENABLE == STD_TRUE)
	if(++TouchRstTimes > 2000)
	{
		Touch_Init();
		TouchTimer64ms = 10;
	}
#endif
}*/

void Touch_TimerProc(void)
{

#if (defined TOUCH_ENABLE) && (TOUCH_ENABLE == STD_TRUE)
	if(TouchTimer64ms != 0)
	{
		TouchTimer64ms--;
		if(TouchTimer64ms == 0)
		{
			Event_Set(EVE_TOUCH_TIMEOUT);
			if(Sys_EnableBlueTooth())
			{
			        ble_dahao_start_advert(1);
			}
		}
		if(Event_Get(EVE_ALARM_SHORT))
		{
			Event_Remove(EVE_ALARM_SHORT);
			Access_BeepStart(BEEP_FAST,6);
			return;
		}
		if(Event_Get(EVE_MANUAL_SHORT))
		{
			Event_Remove(EVE_MANUAL_SHORT);
			Access_Unlock();
		}
	}
#endif
	if(Event_Get(EVE_CONFIG_LONG))
	{
	//	Event_Remove(EVE_CONFIG_LONG);
	//	Access_Globle.DefaultTimes = 0;
	//	Sys_PataCfg.LongKeyFlag = 0x5A;
	//	Sys_Parainit();
	//	Sys_StorePara();
	//	Sys_McuRestFlag = STD_TRUE;//复位
	//	Access_BeepStart(BEEP_FAST,3);
	}
}
uint8 initpsw[3] = {0x12,0x34,0x56};    //  {0x12,0x33,0x21};

/****************************************************************************************************
**Function:
	void HomeLock_Unlock(void)
**Author: rory
**Description:
**Input: 
**Output: 
****************************************************************************************************/
void Touch_KeyProc(uint8* pTouchKey,uint8 lenth)
{
	uint8 ret = E_NOT_OK;
#if (defined RTC_EN) && (RTC_EN == STD_TRUE)
	ProtoAnaly_UpdateTime();
#endif
#if (defined DEBUG_ENABLE) && (DEBUG_ENABLE == STD_TRUE)
	Sys_Debug(STD_OFF);
#endif
    if (0 == memcmp(initpsw,pTouchKey,3 )&& (Sys_PataCfg.Pswd == 0x12345678))
      {
        Access_Unlock();
		return;
      }
	if(lenth>=4)
	{
		ret = LockKey_Check_PaswdKey(pTouchKey, lenth);
	}
	if(ret == E_NOT_OK)
	{
		if(lenth>=8)
		{
			ret = sys_CheckGetDynamicDate(Sys_PataCfg.Mac, Sys_PataCfg.Pswd, pTouchKey, lenth);
			if(ret == E_NOT_OK)
			{
				ret = sys_CheckGetDynamicFlat(Sys_PataCfg.Mac, Sys_PataCfg.Pswd, pTouchKey, lenth);
			}
			if(ret == E_NOT_OK)
			{
				//ret = sys_CheckGetDynamicHotel(Sys_PataCfg.Mac, Sys_PataCfg.Pswd, pTouchKey, lenth);
				ret = sys_CheckGetDynamicHotel_V8(Sys_PataCfg.Mac, Sys_PataCfg.Pswd, pTouchKey, lenth);
			}
		}
		/*else if(lenth>=7)
		{
			ret = sys_CheckGetDynamicHotel(Sys_PataCfg.Mac, Sys_PataCfg.Pswd, pTouchKey, lenth);
		}*/
	}
#if (defined DEBUG_ENABLE) && (DEBUG_ENABLE == STD_TRUE)
	Sys_Debug(STD_ON);
#endif
	if(ret == E_OK)
	{
		Access_Unlock();
	}
	else if(ret == E_END)
	{
		Access_BeepStart(BEEP_VERY_SLOW,1);
		Access_LightStart(LIGHT_OK,LIGHT_SLOW,1,0);
	}
	else if(ret == E_TIME_OUT)
	{
		Access_LightStart(LIGHT_NG,LIGHT_SLOW,1,0);
		Access_BeepStart(BEEP_FAST,BEEP_ERROR_TIME_OUT);
	}
	else
	{
		Access_OpenError();
		Access_Globle.ErrorTimes++;
		if(Access_Globle.ErrorTimes >= 5)
		{
			Access_LockDeviceTimer64ms = 180000/64;
		}
	}
}


uint8 TT;

void Touch_Proc(void)
{
	uint8 dat = 0xff;

#if ((defined TOUCH_WAKEUP_PANEL) && (TOUCH_WAKEUP_PANEL == STD_TRUE))
	if(TouchState == TOUCH_STATE_IDLE)
	{
		TouchTimer64ms = TOUCH_TIME_OUT_DATA;
		TouchState = TOUCH_STATE_INPUT;
		return;
	}
#endif
	if(Event_Get(EVE_TOUCH_TIMEOUT))
	{
		Event_Remove(EVE_TOUCH_TIMEOUT);
		Touch_Clear();
		return;
	}
	if(Event_Get(EVE_0_SHORT))
	{
		Event_Remove(EVE_0_SHORT);
		dat = 0;
	}
	else if(Event_Get(EVE_1_SHORT))
	{
		Event_Remove(EVE_1_SHORT);
		dat = 1;
	}
	else if(Event_Get(EVE_2_SHORT))
	{
		Event_Remove(EVE_2_SHORT);
		dat = 2;
	}
	else if(Event_Get(EVE_3_SHORT))
	{
		Event_Remove(EVE_3_SHORT);
		dat = 3;
	}
	else if(Event_Get(EVE_4_SHORT))
	{
		Event_Remove(EVE_4_SHORT);
		dat = 4;
	}
	else if(Event_Get(EVE_5_SHORT))
	{
		Event_Remove(EVE_5_SHORT);
		dat = 5;
	}
	else if(Event_Get(EVE_6_SHORT))
	{
		Event_Remove(EVE_6_SHORT);
		dat = 6;
	}
	else if(Event_Get(EVE_7_SHORT))
	{
		Event_Remove(EVE_7_SHORT);
		dat = 7;
	}
	else if(Event_Get(EVE_8_SHORT))
	{
		Event_Remove(EVE_8_SHORT);
		dat = 8;
	}
	else if(Event_Get(EVE_9_SHORT))
	{
		Event_Remove(EVE_9_SHORT);
		dat = 9;
	}
	if(dat != 0xff)
	{
	TT = dat;
		if(TouchKeyIndex >= TOUCH_KEY_BUF_MAX)
		{
			for(uint8 i=0; i<(TOUCH_KEY_BUF_MAX>>1); i++)
			{
				uint8 temp;
				temp = TouchBuf[i]<<4; // 移走高4位
				temp |= TouchBuf[i+1]>>4;
				TouchBuf[i] = temp;
			}
			TouchBuf[(TOUCH_KEY_BUF_MAX>>1)-1] &= 0xf0;
			TouchBuf[(TOUCH_KEY_BUF_MAX>>1)-1] |= dat;
			Access_BeepStart(BEEP_FAST,1);         //  BEEP_LOW
		}
		else
		{
			TouchTimer64ms = TOUCH_TIME_OUT_DATA;
			TouchBuf[(TouchKeyIndex>>1)]<<= 4;
			TouchBuf[(TouchKeyIndex>>1)] |= dat;
			TouchKeyIndex++;
			Access_BeepStart(BEEP_FAST,1);         //
		}
		if(Sys_EnableBlueTooth())
		{
			if(TouchState == TOUCH_STATE_IDLE)
		       {
		       	TouchState = TOUCH_STATE_INPUT;
		        	ble_dahao_start_advert(1);
			}
			else
			{
				ble_dahao_start_advert(0);
			}
		}
		return;
	}
	if(Event_Get(EVE_DOORBELL_SHORT))
	{
		Event_Remove(EVE_DOORBELL_SHORT);
		memset(TouchBuf,0xff,sizeof(TouchBuf));
		TouchKeyIndex = 0;
		Access_BeepStart(BEEP_FAST,1);
	}
	if(Event_Get(EVE_ENTER_SHORT))
	{
		Event_Remove(EVE_ENTER_SHORT);
		if(Sys_PataCfg.State == WORK_NORMALLY_OPEN)
		{
			Access_BeepStart(BEEP_LOW,BEEP_ERROR_EMERGENCY);
			return;
		}
		if(TouchKeyIndex <4)
		{
			//Access_OpenError();
			Access_BeepStart(BEEP_FAST,1);
			return;
		}
		if(TouchKeyIndex&0x01)
		{
			TouchBuf[(TouchKeyIndex>>1)]<<= 4;
		}
		Touch_KeyProc(TouchBuf,TouchKeyIndex);
		memset(TouchBuf,0xff,sizeof(TouchBuf));
		TouchKeyIndex = 0;
	}
	else if(Event_Get(EVE_ENTER_LONG))
	{
		Event_Remove(EVE_ENTER_LONG);
		Access_Lock();
		return;
	}
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
