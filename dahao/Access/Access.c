/*****************************************************************************
|  File Name: Data.c
|
|  Description: 
|
|-----------------------------------------------------------------------------
|               C O P Y R I G H T
|-----------------------------------------------------------------------------
| Copyright (c) 2002-2012 by DaHao TECHNOLOGY Co., Ltd. All rights reserved.
|
|               This software is copyright protected and proprietary 
|               to DaHao TECHNOLOGY Co., Ltd. DaHao TECHNOLOGY Co., Ltd 
|               grants to you only those rights as set out in the 
|               license conditions. All other rights remain with 
|               DaHao TECHNOLOGY Co., Ltd.
|-----------------------------------------------------------------------------
|               A U T H O R   I D E N T I T Y
|-----------------------------------------------------------------------------
| Initials     Name                      Company
| --------     ---------------------     -------------------------------------
| LXW          LXW               DaHao TECHNOLOGY Co., Ltd
|-----------------------------------------------------------------------------
|               R E V I S I O N   H I S T O R Y
|-----------------------------------------------------------------------------
| Date        Ver     Author  Description
| ---------   ----    ------  ------------------------------------------------
| 2013-12 1.00    Rory     Initial version 
|****************************************************************************/

/**************************************************************************************************/ 
/* Include file                                                                                   */                                                 
/**************************************************************************************************/
//#include <absacc.h> 
#include "SYSTEM\sys.h"
//#include "UART\UART.h"
#include "KEY\key.h"
#include "SYSTEM\Sys_TimeBase.h"
#include "Protocol\factInfo.h"
#include "Access\Access.h"
#include "Pwm\Pwm.h"
#include "Card\Rc522.h"
#include "protocol\Proto_Analy.h"
#include "RTC\Rtc.h"
#include "Flash\Flash.h"
#include "KEY\Touch.h"
#include "Access\LockKey.h"
#include "Commu\Lock_Commu.h"
#include "Light\light.h"
#include "adc\adc_Nrf.h"
#include "ble_dahao.h"
#include "Encpt\Encpt.h"
#include "CustomCode\CustomCode.h"
#include "MOTOR\motor.h"
#include "FACTORY\Factory.h"

Access_ParaType Access_Globle;

uint8 Access_MotorTimer64 = 0;
uint16 Access_LockTimer64 = 0;
uint16 Access_DefaultTimer64 = 0;
uint16 Access_CardLearnTimer64 = 0;
uint16 Access_ResetDeviceTimer64 = 0;

uint8  Access_AddId_NewFlag= 0;
uint8  Access_FlashProcFlag = 0;
uint32 Access_UserId_Temp = 0;
uint32 Access_ScanInex_Temp = 0;
uint8 Access_AttTIme_Temp[9] = {0};


uint16 Access_OpenRedIndex = 0;
uint16 Access_OpenWriteaIndex = 0;

uint16 Access_Store_Recordx64ms = 0;
uint8 Access_KeyDatTimex64ms=0 ;
uint16 Access_BatteryData;
uint16 Access_LockDeviceTimer64ms=0;  /* 三次错误锁住*/

uint8 Access_LockStatus=0; /* 门锁状态*/

uint8 	RelayOpenFlag;
uint8	RelayOpenDelayTime;
uint8 	HardDbgFlag;
uint8 	HardDbgKeyFlag;


AccRcordParaType AccRcord;
//Access_SyncPaswdType Access_SyncPaswd;

void Access_CardProcess(uint8 idtpye, uint8* pUid,Access_CardDatType *CardDat);
Std_ReturnType Access_ComTimeLoop(uint32 startTime, uint32 endTime);

extern void Tsm_Init(void);
extern void Touch_Init( void );

extern Rtc_Type ProtoAnaly_Rtcinfo;
extern uint8  Cardetflag;
extern uint8  Lora_Timeout;
extern uint8  ProtoAnaly_LoraBrgSize;
extern uint8 SysSleepFlag;
//extern uint8  Sys_frstPoweron;
extern uint8 BattdetTimer;
/****************************************************************************************************
**Function:
	void Access_Init(void)
**Author: rory
**Description:
**Input: 
**Output: 
****************************************************************************************************/
void Access_Init(void)
{
	Access_LockTimer64 = 0;
	Access_DefaultTimer64 = 0;
	Access_CardLearnTimer64 = 0;
	Access_ResetDeviceTimer64 = 0;

	if(Sys_PataCfg.State  == WORK_LEARN_CARD)
	{
		Sys_PataCfg.State = WORK_NORMAL;
	}
	Access_Globle.DefaultTimes = 0;
	LockCommu_Init();
	BlackList_LoadPara();
	//ServeKeyList_LoadPara();
	RecordList_LoadPara();
	Access_FlashArrang();
#if (defined TOUCH_ENABLE) && (TOUCH_ENABLE == STD_TRUE)	
	TouchRstTimes = 1;
#endif
	Factory_State = COMD_FATORY_IDLE;
}

/****************************************************************************************************
**Function:
	void Access_DetCardProc(Card_ParmType *pdat, Access_CardDatType *pAccess_CardDat)
**Author: rory
**Description:  64ms  once
**Input: 
**Output: 
****************************************************************************************************/
uint8 Access_CardDatProc(Card_ParmType *pdat, Access_CardDatType *pAccess_CardDat)
{
	Encpt_DecrpytExt(&pdat->pdat[0]);
	Encpt_DecrpytExt(&pdat->pdat[8]);

	if((pdat->pdat[0] == 0)&&(pdat->pdat[1] == 0))
	{	
		return E_OK;
	}
	else 
	{   
		uint8 year, month, day, hour, minute;
		uint8 storeFlag;
		
		uint8 cardType = pdat->pdat[0] >> 5;
		pAccess_CardDat->hotelId = BUILD_UINT16(pdat->pdat[1], pdat->pdat[0])&0x1fff;
		/* start Time */
		year = (pdat->pdat[10]>>1);
		year += 16;
		month = ((pdat->pdat[10]&0x01)<<3)|(pdat->pdat[11]>>5);
		day = pdat->pdat[11]&0x1f;
		hour = pdat->pdat[12]/4;
		minute = (pdat->pdat[12]%4)*15;
		pAccess_CardDat->Timest = BUILD_TIME(year, month, day, hour, minute, 0);

		/* end Time */
		year = (pdat->pdat[13]>>1);
		year += 16;
		month = ((pdat->pdat[13]&0x01)<<3)|(pdat->pdat[14]>>5);
		day = pdat->pdat[14]&0x1f;
		hour = pdat->pdat[15]/4;
		minute = (pdat->pdat[15]%4)*15;
		
		pAccess_CardDat->Timeend = BUILD_TIME(year, month, day, hour, minute, 0);
		pAccess_CardDat->cardtype = CARD_TYPE_NULL;
		pAccess_CardDat->HotelPaswd = BUILD_UINT16(pdat->pdat[3], pdat->pdat[2])&0x0fff;
		if(cardType == 2)
		{  /* 客人卡*/
			if((pAccess_CardDat->hotelId != Sys_PataCfg.PropertyId) || (Sys_PataCfg.PropertyId == 0))
			{
			return E_OK;
			}
			if((Access_ComTime(pAccess_CardDat->Timest, pAccess_CardDat->Timeend) != E_OK)
				|| (pAccess_CardDat->Timest < AccRcord.CustomTimerBk))
			{/*卡片失效*/
				return E_TIME_OUT;
			}
			pAccess_CardDat->deviceId = BUILD_UINT32(pdat->pdat[5], pdat->pdat[4], pdat->pdat[3], pdat->pdat[2]);
			pAccess_CardDat->devicePaswd = BUILD_UINT32(pdat->pdat[9], pdat->pdat[8], pdat->pdat[7], pdat->pdat[6]);
			pAccess_CardDat->cardtype = CARD_TYPE_GUEST;
			if(pAccess_CardDat->Timest > AccRcord.CustomTimerBk)
			{
				AccRcord.CustomTimerBk = pAccess_CardDat->Timest;
				RecordList_StorePara(0);
			}
			return E_OK;
		}
		else if(cardType == 4)
		{/* 员工卡*/
			uint8 type = (pdat->pdat[2]&0xf0)>>4;
			uint8 configFlag  = pdat->pdat[8];
			
			if((pAccess_CardDat->hotelId != Sys_PataCfg.PropertyId) || (Sys_PataCfg.PropertyId == 0)
			|| (pAccess_CardDat->HotelPaswd != Sys_PataCfg.HotelPswd))
			{
				return E_OK;
			}
			if(Access_ComTime(pAccess_CardDat->Timest, pAccess_CardDat->Timeend) != E_OK)
			{/*卡片失效*/
				if((type != CARD_TYPE_STAFF_LOOP) && (type != CARD_TYPE_LOSS))
				{
					return E_TIME_OUT;
				}
			}
			
			if(type == CARD_TYPE_INSPECTION)
			{/* 巡检测卡*/
			#if (defined(SUPPORT_RECORD_LOC_STORE)&&(SUPPORT_RECORD_LOC_STORE == STD_TRUE))
				Access_WriteRecordFlash(pdat->pid,ProtoAnaly_RtcLocalTime,RECORD_INSPECTION_CARD,ACCESS_OPEN_LOCK_TPYE);
			#endif
			#if (SUPPORT_RECORD_RTIME_SEND == STD_TRUE)
				ProtoAnaly_AddRecordCmd(pdat->pid,ProtoAnaly_RtcLocalTime,RECORD_INSPECTION_CARD,ACCESS_OPEN_LOCK_TPYE);
			#endif
				return E_END_LIGHT;
			}
			if(type == CARD_TYPE_LOSS)
			{/* 挂失卡*/
				pAccess_CardDat->deviceId = BUILD_UINT32(pdat->pdat[9], pdat->pdat[8], pdat->pdat[7], pdat->pdat[6]);
				BlackList_Add(KEY_TYPE_CARD, pAccess_CardDat->deviceId, BUILD_TIME(year, month, day, hour, 59, 59));
				BlackList_StorePara();
				return E_END;
			}
			if(type == CARD_TYPE_EMERGENCY)
			{/* 应急卡*/
				if(pAccess_CardDat->Timest < AccRcord.EmergencyTimerBk)
				{
					return E_TIME_OUT;
				}
				pAccess_CardDat->cardtype = CARD_TYPE_EMERGENCY;
				if(configFlag & LOSS_OLD_CARD)
				{
					if(pAccess_CardDat->Timest > AccRcord.EmergencyTimerBk)
					{
						AccRcord.EmergencyTimerBk = pAccess_CardDat->Timest;
						RecordList_StorePara(0);
					}
				}
				return E_OK;
			}
			if(type == CARD_TYPE_ALL)
			{/* 总卡*/
				if(pAccess_CardDat->Timest < AccRcord.AllTimerBk)
				{
					return E_TIME_OUT;
				}
				pAccess_CardDat->cardtype = CARD_TYPE_ALL;
				if(configFlag & LOSS_OLD_CARD)
				{
					if(pAccess_CardDat->Timest > AccRcord.AllTimerBk)
					{
						AccRcord.AllTimerBk = pAccess_CardDat->Timest;
						RecordList_StorePara(0);
					}
				}
				return E_OK;
			}
			if(Sys_PataCfg.BuildId == 0)
			{
				pAccess_CardDat->cardtype = type;
				return E_OK;
			}
			else
			{
				uint8 i;
					
				/*if((pdat->pdat[4] == 0) && (pdat->pdat[5] == 0) && (pdat->pdat[6] == 0)
					&& (pdat->pdat[7] == 0))
				{
					pAccess_CardDat->cardtype = type;
				}
				else*/
				{
					if(pAccess_CardDat->Timest < AccRcord.StaffTimerBk)
					{
						return E_TIME_OUT;
					}
					for(i=0; i<ACCESS_CARDBUILDID_MAX; i++)
					{
						if(Sys_PataCfg.BuildId == pdat->pdat[4+i])
						{
								//pAccess_CardDat->cardtype = type;
							break;
						}
					}
					if(i >= ACCESS_CARDBUILDID_MAX)
					{
						return E_NOT_OK;
					}
					if(configFlag & LOSS_OLD_CARD)
					{
						if(pAccess_CardDat->Timest > AccRcord.StaffTimerBk)
						{
							AccRcord.StaffTimerBk = pAccess_CardDat->Timest;
							RecordList_StorePara(0);
						}
					}
				}
				if(type == CARD_TYPE_STAFF)
				{/* 时间限制*/
					pAccess_CardDat->cardtype = type;
					return E_OK;
				}
				else if(type == CARD_TYPE_STAFF_LOOP)
				{
					uint8 t = 0;

					if(ProtoAnaly_Rtcinfo.Week == 7)
					{
						t = 0;
					}
					else
					{
						t = ProtoAnaly_Rtcinfo.Week;
					}
					
					//for(i=0; i<8; i++)
					{
						if(pdat->pdat[9] & (1<<t))
						{
							//if(ProtoAnaly_Rtcinfo.Week == i)
							{
								if(Access_ComTimeLoop(pAccess_CardDat->Timest, pAccess_CardDat->Timeend) == E_OK)
								{
									pAccess_CardDat->cardtype = type;
									return E_OK;
								}
								else
								{
									return E_NOT_OK;
								}
							}
						}
					}
				}
				pAccess_CardDat->cardtype = CARD_TYPE_NULL;
				return E_NOT_OK;
			}
		}
		else if(cardType == 6)
		{/* 门锁配置卡*/
			if((pdat->pdat[2]&0xf0) == 0x00)
			{/* 门锁配置*/
				if((BUILD_UINT32(pdat->pdat[9], pdat->pdat[8], pdat->pdat[7], pdat->pdat[6])&0x0000FFFF) != 0x00003344)
			{/* 验证卡片合法性*/
					return E_NOT_OK;
				}
			}
			else
			{
				if((BUILD_UINT32(pdat->pdat[9], pdat->pdat[8], pdat->pdat[7], pdat->pdat[6])&0xFFFFFFFF) != 0x11223344)
				{/* 验证卡片合法性*/
					return E_NOT_OK;
				}
			}
			if(Access_ComTime(pAccess_CardDat->Timest, pAccess_CardDat->Timeend) != E_OK)
			{/*卡片失效*/
				return E_TIME_OUT;
			}
			if((pdat->pdat[2]&0xf0) == 0x00)
			{/* 门锁配置*/
				uint16 newPaswd;

				if((Sys_PataCfg.PropertyId == 0)&&(Sys_PataCfg.Pswd != 0x12345678))
				{
					return E_OK;
				}
				if(Sys_PataCfg.PropertyId == 0)
				{/* 重未配置过*/
					Sys_PataCfg.PropertyId = pAccess_CardDat->hotelId;
					Sys_PataCfg.HotelPswd = pAccess_CardDat->HotelPaswd;
					if(pdat->pdat[6] != 0xff)
					{
						Sys_PataCfg.BuildId = pdat->pdat[6];
					}
					if(pdat->pdat[7] < 0x10)
					{
						Sys_PataCfg.usedpage = pdat->pdat[7];
					}
					Sys_StorePara();
					Sys_McuRestFlag=STD_TRUE;
					return E_END;
				}
				else
				{
					if(/*(pAccess_CardDat->HotelPaswd != Sys_PataCfg.HotelPswd) ||*/(pAccess_CardDat->hotelId != Sys_PataCfg.PropertyId))
					{
						return E_OK;
					}
				}
				storeFlag = 0;
				newPaswd = BUILD_UINT16(pdat->pdat[5], pdat->pdat[4])&0x0fff;
					Sys_PataCfg.HotelPswd = newPaswd;
				if(pdat->pdat[6] != 0xff)
				{
					Sys_PataCfg.BuildId = pdat->pdat[6];
				}
				if(pdat->pdat[7] < 16)
				{
					Sys_PataCfg.usedpage = pdat->pdat[7];
					storeFlag = 1;
				}
					Sys_StorePara();
					Sys_McuRestFlag=STD_TRUE;
				return E_END;
			}
		}
		else if(cardType == 5)
		{/* 配置卡*/
			if(BUILD_UINT32(pdat->pdat[9], pdat->pdat[8], pdat->pdat[7], pdat->pdat[6]) != 0x11223344)
			{
				return E_OK;
			}
			if((pdat->pdat[2]&0xf0) == 0x10)
			{/* 功能配置*/
				if(pdat->pdat[3] == 1)
				{/* 开蓝牙*/
					Sys_PataCfg.CfgFlag &= ~DISIBLEBLUETOOTH;
				}
				else if(pdat->pdat[3] == 0)
				{/* 关蓝牙*/
					Sys_PataCfg.CfgFlag |= DISIBLEBLUETOOTH;
				}
				else if(pdat->pdat[3] == 2)
				{/* 唤醒蓝牙*/
					 if(Sys_EnableBlueTooth())
				 	{
						ble_dahao_start_advert(0);
					}
					return E_END;
				}
				Sys_StorePara();
				Sys_McuRestFlag=STD_TRUE;
				return E_END;
			}
			else if((pdat->pdat[2]&0xf0) == 0x20)
			{/* 客户代码*/
				if(Access_ComTime(pAccess_CardDat->Timest, pAccess_CardDat->Timeend) != E_OK)
				{/*卡片失效*/
					return E_TIME_OUT;
				}
				return E_END;
			}
			else if((pdat->pdat[2]&0xf0) == 0x30)
			{/* 工厂代码*/
				if(Access_ComTime(pAccess_CardDat->Timest, pAccess_CardDat->Timeend) != E_OK)
				{/*卡片失效*/
					return E_TIME_OUT;
				}
				Sys_PataCfg.CustomId = BUILD_UINT16(pdat->pdat[5], pdat->pdat[4]);
				Sys_StorePara();
				Sys_McuRestFlag=STD_TRUE;
				return E_END;
			}
			else  if((pdat->pdat[2]&0xf0) == 0x40)
			{/* 工厂测试卡*/
				if(Access_ComTime(pAccess_CardDat->Timest, pAccess_CardDat->Timeend) != E_OK)
				{/*卡片失效*/
					return E_TIME_OUT;
				}
				if(Sys_PataCfg.Pswd != 0x12345678)
				{
					return E_END;
				}
				pAccess_CardDat->cardtype = CARD_TYPE_TESK;
				return E_OK;
			}
		}
		else
		{
			return E_OK;
		}
	}
	return E_OK;
}

	

/****************************************************************************************************
**Function:
	void Access_DetCardProc(void)
**Author: rory
**Description:  64ms  once
**Input: 
**Output: 
****************************************************************************************************/
void Access_DetCardProc(void)
{
	uint8 CardId[8] = {0};
	uint8 Carddat[16];
	Card_ParmType pcard;
	Access_CardDatType Access_CardDat;
	uint8 ret;

		pcard.pid = CardId;
		pcard.pdat = Carddat;
#if (RC522_EN == STD_TRUE)
		memset(Carddat,0,sizeof(Carddat));
		if(Rc522Drive.read(&pcard) == E_OK)
		{ 
		#if (defined DEBUG_ENABLE) && (DEBUG_ENABLE == STD_TRUE)
			Sys_Debug(STD_OFF);
		#endif
		#if (defined RTC_EN) && (RTC_EN == STD_TRUE)
			ProtoAnaly_UpdateTime();
		#endif
#if (RC522_RDDAT == STD_TRUE)
			memset((uint8*)&Access_CardDat, 0xff, sizeof(Access_CardDat));
			ret = Access_CardDatProc(&pcard, &Access_CardDat);
			if(ret == E_END)
			{
				Access_BeepStart(BEEP_NORMAL,1);	
				Carddet_timer_start(500);
				Access_LightStart(LIGHT_OK,LIGHT_SLOW,1,0);
				return;
			}
			else if(ret == E_END_LIGHT)
			{
				Carddet_timer_start(500);
				Access_LightStart(LIGHT_OK,LIGHT_SLOW,1,0);
				return;
			}
			else if(ret == E_NOT_OK)
			{
				Access_BeepStart(BEEP_NORMAL,2);	
				Access_LightStart(LIGHT_NG,LIGHT_SLOW,1,0);
				Carddet_timer_start(500);
				return;
			}
			else if(ret == E_TIME_OUT)
			{
				Access_BeepStart(BEEP_NORMAL,BEEP_ERROR_TIME_OUT);	
				Access_LightStart(LIGHT_NG,LIGHT_SLOW,1,0);
				Carddet_timer_start(500);
				return;
			}
			else if(ret == E_OK)
#endif
			{
				goto CARDIDPROC;
			}
		}
#endif			
#if (defined DEBUG_ENABLE) && (DEBUG_ENABLE == STD_TRUE)
	Sys_Debug(STD_ON);
#endif
	if(Sys_State == SYS_SLEEP)
	{
		if(Sys_CheckCardSleepDet())
		{
			Carddet_timer_start(300);
		}
	}
	else 
	{
		Carddet_timer_start(300);
	}
	return;
	CARDIDPROC:
	if(Access_GetSupportType(pcard.IdType) ==E_OK)
	{
		uint8 tbuf[8]={0};
		if(pcard.IdType == KEY_TYPE_CARD)
		{
			tbuf[0]= CardId[3];
			tbuf[1]= CardId[2];
			tbuf[2]= CardId[1];
			tbuf[3]= CardId[0];
		}
		else 
		{
			tbuf[0]= CardId[0];
			tbuf[1]= CardId[1];
			tbuf[2]= CardId[2];
			tbuf[3]= CardId[3];
			tbuf[4]= CardId[4];
			tbuf[5]= CardId[5];
			tbuf[6]= CardId[6];
			tbuf[7]= CardId[7];
		}
		Access_CardProcess(pcard.IdType,tbuf,&Access_CardDat);
		Access_KeyDatTimex64ms = 100/64;
		Carddet_timer_start(500);
	}
#if (defined DEBUG_ENABLE) && (DEBUG_ENABLE == STD_TRUE)
	Sys_Debug(STD_ON);
#endif
}
/****************************************************************************************************
**Function:
	void Access_Proc(void)
**Author: rory
**Description:  64ms
**Input: 
**Output: 
****************************************************************************************************/
void Access_TimerProc(void)
{	
	/*if(Access_MotorTimer64 != 0)
	{
		Access_MotorTimer64--;
		if(Access_MotorTimer64 == 0)
		{
			Access_MotorStop();
		}
	}*/
	if(Access_ResetDeviceTimer64 != 0)
	{
		Access_ResetDeviceTimer64--;
		if(Access_ResetDeviceTimer64 == 0)
		{
			NVIC_SystemReset();
		}
	}
#if 0
	if(Access_LockTimer64 != 0)
	{
		Access_LockTimer64--;
		if(Access_LockTimer64 == 0)
		{
			Access_MotorClose();
#if (defined TOUCH_ENABLE) && (TOUCH_ENABLE == STD_TRUE)	
			TouchRstTimes = 1;
#endif
		}
	}
#endif
	if(Access_LockDeviceTimer64ms != 0)
	{
		Access_LockDeviceTimer64ms--;
		if(Access_LockDeviceTimer64ms <= 1)
		{
			Access_Globle.ErrorTimes =0;
			Access_LockDeviceTimer64ms = 0;
		}
	}
	
	if(Access_CardLearnTimer64 != 0)
	{
		--Access_CardLearnTimer64;
		if(Access_CardLearnTimer64 == 0)
		{
			Sys_PataCfg.State = WORK_NORMAL;
			Access_BeepStart(BEEP_NORMAL,1);
		}
	}
#if 0	
	if(++Sys_SleepErrTimsoutx64ms >= (60000/64) *10)   
	{
		// 超时未休眠
		Sys_SleepErrTimsoutx64ms = 0;
		NVIC_SystemReset();
	}
#endif	
	BeepDrive.ioctl(BEEP_CMD_SCAN, NULL);
	LightDrive.ioctl(LIGHT_CMD_SCAN,NULL);
}
/****************************************************************************************************
**Function:
	void Access_MotorOpen(void)
**Author: rory
**Description:
**Input: 
**Output: 
****************************************************************************************************/
void Vdd_PowerOff(void)
{
	if(BattdetTimer == STD_FALSE)
	{
		nrf_gpio_pin_clear(VDD_POWER);
	}
}
#if 0
/****************************************************************************************************
**Function:
	void Access_MotorOpen(void)
**Author: rory
**Description:
**Input: 
**Output: 
****************************************************************************************************/
void Access_MotorOpen(void)
{
	Vdd_PowerOn();
	nrf_gpio_cfg_output(LOCK_M_ADD_PIN);
	nrf_gpio_cfg_output(LOCK_M_MINUS_PIN);
	nrf_gpio_pin_clear(LOCK_M_ADD_PIN); 
	nrf_gpio_pin_set(LOCK_M_MINUS_PIN);
	Access_MotorTimer64 = Sys_PataCfg.MotorTime/64;
}
/****************************************************************************************************
**Function:
	void Access_MotorClose(void)
**Author: rory
**Description:
**Input: 
**Output: 
****************************************************************************************************/
void Access_MotorClose(void)
{
	Vdd_PowerOn();
	nrf_gpio_pin_set(LOCK_M_ADD_PIN); 
	nrf_gpio_pin_clear(LOCK_M_MINUS_PIN);
	Access_MotorTimer64 = Sys_PataCfg.MotorTime/64;
/*#if (defined DETECT_SW_EN) && (DETECT_SW_EN == STD_TRUE)
	SensorDrive.ioctl(SENSOR_CMD_FALSE_LOCK, NULL);
#endif*/
       BattdetTimer = STD_FALSE;
	Battdet_timer_start(500);
}

/****************************************************************************************************
**Function:
	void Access_MotorStop(void)
**Author: rory
**Description:
**Input: 
**Output: 
****************************************************************************************************/
void Access_MotorStop(void)
{
	nrf_gpio_cfg_output(LOCK_M_ADD_PIN);
	nrf_gpio_cfg_output(LOCK_M_MINUS_PIN);

	nrf_gpio_pin_clear(LOCK_M_ADD_PIN); 
	nrf_gpio_pin_clear(LOCK_M_MINUS_PIN);
	Vdd_PowerOff();
}
#endif


void ReLay_Keyscan(void)                         //  debug
{
	static uint8 cnt;
	nrf_gpio_cfg_input(RELAY_SW_PIN,NRF_GPIO_PIN_PULLUP);
	if ( RELAYSWGetData() == 0 && 0 == RelayOpenFlag )    // RELAYSWGetData 
	  {
	    if ( ++cnt >= 2 )
	      {
	        cnt = 0;
	        Access_Unlock();
	      }
	  }
}

static uint8 InfraredFlag;

void Infrared_Keyscan(void)                      
{
	static uint8 cnt;
	nrf_gpio_cfg_input(Infrared_PIN,NRF_GPIO_PIN_PULLUP);
	
	if ( InfraredGetData() == 1 )    
	  {
	    if ( ++cnt >= 2 && 0 == InfraredFlag )
	      {
	        cnt = 0;
			InfraredFlag = 1;
			Access_BeepStart(BEEP_VERY_FAST,5);
			Access_LightStart(LIGHT_OK,LIGHT_SLOW,1,0);
	      }
	  }
	else
	  {
         InfraredFlag = 0;
	  }
		
}

static Rtc_Type Rtc_dataDbg1;
static Rtc_Type Rtc_dataDbg2;
static uint8 DbgBuf[4] ={0x5A,0xA5,0x5A,0xA5};

void HardDbg_Timepro(void) 
{
	static uint16 cnt,cnt1;
    if ( 0 == HardDbgFlag )
      {
         if ( CfgKeyGetData() == 0 && HardDbgKeyFlag == 0 )
           {
             HardDbgKeyFlag = 1;
           }
		 if (Sys_PataCfg.LongKeyFlag == 0x5A)
		   {
		     if ( ++cnt >= 100 )   //  4S
		       {
		         cnt = 0;
		         Sys_PataCfg.LongKeyFlag = 0;
				 Sys_StorePara();
		       }
		   }

		 if ( HardDbgKeyFlag && CfgKeyGetData() == 1)
		   {
		     if ( Sys_PataCfg.LongKeyFlag != 0x5A )
		       {
			     HardDbgFlag = 1;
				 HardDbgKeyFlag = 0;
				 Access_BeepStart(BEEP_VERY_FAST,5);
				 HC595_LedWriteData(0xffff);
				 Rtc_Read(&Rtc_dataDbg1);
		       }
		   }
      }

	if ( HardDbgFlag )
	  {
	    SendUart_data(DbgBuf,sizeof(DbgBuf));
		Send_Weigand34(&DbgBuf);
        cnt1++;
		if ( cnt1 == 40 )
		  {
			if ( 0xEF15 == SPI_FLASH_ReadID())
			  {
			    HC595_LedWriteData(0x0010|0x8000);    // green
				Access_BeepStart(BEEP_OPEN,1);
			  }
			else
			  {
			    HC595_LedWriteData(0x0010|0x2000);   // red	1
				Access_BeepStart(BEEP_VERY_FAST,3);			  
			  }
		  }
		else if ( cnt1 == 60 )
		  {
			 Rtc_Read(&Rtc_dataDbg2);
			 
		     if ( Rtc_dataDbg2.Second > Rtc_dataDbg1.Second )
		       {
				 HC595_LedWriteData(0x0008|0x8000);	// green
				 Access_BeepStart(BEEP_OPEN,1);
		       }
			 else
			   {
			   	  HC595_LedWriteData(0x0008|0x2000);   // red	2
				  Access_BeepStart(BEEP_VERY_FAST,3);	
			   }
		    
		  }
		else if( cnt1 >= 80 && cnt1 < 100)
		  {
			if ( 1 == InfraredGetData())
			  {
			    HC595_LedWriteData( 0x0100|0x2000 );      // red
			  }
			else
			  {
			    HC595_LedWriteData( 0x0100|0x8000 );      // green 
			  }
		  }
		else if ( cnt1 >= 100 )
		  {
		    cnt1 = 0;
		  	HardDbgFlag = 0;
			Access_BeepStart(BEEP_NORMAL,1);
			HC595_LedWriteData(0x00);
		  }
	  }
}

void test_Timepro(void) 
  {
    static uint16 cnt;
	
	cnt++;
   if ( cnt == 20 )
	{
		HC595_LedWriteData( 0x0080|0x8000|0x2000|0x0400 );	 // * green red
	}
	else if ( cnt >= 23 )
	{
	   cnt = 0;
	   HC595_LedWriteData(0x00);
	}
  }

void ReLay_Timepro(void) 
{
   
   ReLay_Keyscan();
   Infrared_Keyscan();
   HardDbg_Timepro();
   //test_Timepro();

   if ( RelayOpenFlag )
   	 {
   	   if ( --RelayOpenDelayTime <= 0)
   	   	{
   	   	  RelayOpenFlag = 0;

		  cardopenFlag = 0;
		  ReLay_PIN_Low();
   	   	}
   	 }
}


/****************************************************************************************************
**Function:
	void Access_Unlock(void)
**Author: rory
**Description:
**Input: 
**Output: 
****************************************************************************************************/
void Access_Unlock(void)     //   open zhe door 
{
	Vdd_PowerOn();
	Access_Globle.ErrorTimes =0;
	Access_LockDeviceTimer64ms = 0;

	Access_BeepStart(BEEP_OPEN,1);
	Access_LightStart(LIGHT_OK,LIGHT_SLOW,1,0);

    ReLay_PIN_output();
	ReLay_PIN_High();

	RelayOpenFlag = 1;
	RelayOpenDelayTime = 78;
}
/****************************************************************************************************
**Function:
	void Access_Lock(void)
**Author: rory
**Description:
**Input: 
**Output: 
****************************************************************************************************/
void Access_Lock(void)
{
	Motor_AutoLockTimer64ms = 0;
	Motor_DoorLockSt = MOTOR_DOORLOCK_IDLE;
	Access_Globle.ErrorTimes =0;
	Access_LockDeviceTimer64ms = 0;
	//Access_BeepStart(BEEP_OPEN,1);
	//Access_LightStart(LIGHT_OK,LIGHT_SLOW,1,0);
	//MotorDrive.ioctl(MOTOR_CMD_CLOSE, NULL);
}
/****************************************************************************************************
**Function:
	void Access_OpenError(void)
**Author: rory
**Description:
**Input: 
**Output: 
****************************************************************************************************/
void Access_OpenError(void)
{
	Access_BeepStart(BEEP_VERY_FAST,4);
	Access_LightStart(LIGHT_NG,LIGHT_SLOW,1,0);
}

//#define ACCESS_FASTCMPMAX	1024
//#define ACCESS_FASTCMPMIN	512
//static uint8 Access_FastCmpBuf[ACCESS_FASTCMPMAX] __attribute__((at(0x20002D90)));

/****************************************************************************************************
**Function:
	void Access_EraseAllKey(void)
**Author: rory
**Description: 
**Input: 
**Output: 
****************************************************************************************************/
void Access_EraseAllKey(uint8 idtpye)
{
	memset(&LockKeyBlackList,0xff,sizeof(LockKeyBlackList));
	BlackList_StorePara();

	for (uint8 j=0;j<SERVE_KEY_PAGEMAX;j++ )
	{
		memset(&LockKeyServeKeyList,0xff,sizeof(LockKeyServeKeyList));
		ServeKeyList_Writeflash(j);
	}
	//addr = ACCESS_SERVE_KEY_OFFSET;
    //addr1 = ACCESS_SERVE_KEY_OFFSET + 0x10000; 	
	//FlashDrive.ioctl(FLASH_ERASE_SECT,&addr);
	//FlashDrive.ioctl(FLASH_ERASE_BLOCK,&addr1);
	//ServeKeyList_StorePara1();
}

/****************************************************************************************************
**Function:
	void Access_DefaultPaswd(void)
**Author: rory
**Description: 
**Input: 
**Output: 
****************************************************************************************************/

void Access_DefaultPaswd(void)
{
	LockKeyServeKeyList.KeyList[0].Type = KEY_TYPE_PASSWORD;
	LockKeyServeKeyList.KeyList[0].KeyId = 0xff888888;
	LockKeyServeKeyList.KeyList[0].EndTime = BUILD_TIME(30, 12, 30, 23, 59, 59);
	LockKeyServeKeyList.KeyList[0].StartTime = BUILD_TIME(18, 1, 1, 1, 1, 1);	
	LockKeyStatistics.ServKeyCount++;
	//ServeKeyList_StorePara1();
}

/****************************************************************************************************
**Function:
	void Access_LearnCardOk(uint8 newflag,uint8 idtpye,uint16 index,uint8 *time,uint8 *userid,uint8*cardid)
**Author: lxw
**Description:
**Input: 
**Output: 
****************************************************************************************************/
void Access_LearnCardOk(uint8 idtpye,uint8*cardid)
{
	uint8 index = 0xff;
	uint8 flashoffset;
		
	uint32 cardId;
	uint8 modify = 0;

	if(idtpye == KEY_TYPE_IDENTITY_CARD)
	{
		cardId = BUILD_UINT32(cardid[7],cardid[6],cardid[5],cardid[4]);
	}
	else
	{
		cardId = BUILD_UINT32(cardid[3],cardid[2],cardid[1],cardid[0]);
	}
	
	for(uint8 i=0; i<BLACK_LIST_MAX; i++)
	{
		if((KEY_TYPE_CARD == LockKeyBlackList.BlackList[i].Type) || (KEY_TYPE_IDENTITY_CARD == LockKeyBlackList.BlackList[i].Type))
		{
			if(cardId == LockKeyBlackList.BlackList[i].KeyId)
			{
				modify = 1;
				memset(&LockKeyBlackList.BlackList[i], 0xff, sizeof(LockKey_BlackType));
				if(LockKeyStatistics.BlackListCount > 0)
				{
					LockKeyStatistics.BlackListCount--;
				}
			}
		}
	}
	if(modify == 1)
	{
		BlackList_StorePara();
	}
	for (uint8 j=0;j<SERVE_KEY_PAGEMAX;j++ )
	{
	    ServeKeyList_Readflash( j );
		for(uint8 i=0; i<SERVE_KEY_MAX; i++)
		{
			if(cardId == LockKeyServeKeyList.KeyList[i].KeyId)
			{//强制修改有效时间
				LockKeyServeKeyList.KeyList[i].StartTime = BUILD_UINT32(Access_AttTIme_Temp[3],Access_AttTIme_Temp[2],Access_AttTIme_Temp[1],Access_AttTIme_Temp[0]);
				LockKeyServeKeyList.KeyList[i].EndTime = BUILD_UINT32(Access_AttTIme_Temp[7],Access_AttTIme_Temp[6],Access_AttTIme_Temp[5],Access_AttTIme_Temp[4]);
				LockKeyServeKeyList.KeyList[i].Cycle = Access_AttTIme_Temp[8];
				ServeKeyList_Writeflash( j );
				//ServeKeyList_StorePara1();
				return;
			}
			if(index == 0xff)
			{
				if(LockKeyServeKeyList.KeyList[i].KeyId == 0xffffffff)
				{
					index = i;
					flashoffset = j;
				}
			}
		}
	}
	if(index != 0xff)
	{//增新
	    ServeKeyList_Readflash( flashoffset );
		LockKeyServeKeyList.KeyList[index].Type = idtpye;     // read 
		LockKeyServeKeyList.KeyList[index].KeyId = cardId;
		LockKeyServeKeyList.KeyList[index].StartTime = BUILD_UINT32(Access_AttTIme_Temp[3],Access_AttTIme_Temp[2],Access_AttTIme_Temp[1],Access_AttTIme_Temp[0]);
		LockKeyServeKeyList.KeyList[index].EndTime = BUILD_UINT32(Access_AttTIme_Temp[7],Access_AttTIme_Temp[6],Access_AttTIme_Temp[5],Access_AttTIme_Temp[4]);
		LockKeyServeKeyList.KeyList[index].Cycle = Access_AttTIme_Temp[8];
		LockKeyStatistics.ServKeyCount++;
		ServeKeyList_Writeflash( flashoffset );
		
		//ServeKeyList_StorePara1();
		return;
	}
	return;
}
/****************************************************************************************************
**Function:
	Std_ReturnType Access_ComTime(uint32 startTime, uint32 endTime)
**Author: rory
**Description:
**Input: 
**Output: 
****************************************************************************************************/
Std_ReturnType Access_ComTime(uint32 startTime, uint32 endTime)
{
	uint32 LocalTime = ProtoAnaly_RtcLocalTime;//&0xffffffc0;
	
	if((startTime == 0xffffffff) || (LocalTime < startTime))
	{
		return E_END;
	}
	else if(endTime == 0xffffffff)
	{
		return E_OK;
	}
	if(startTime == 0)
	{
		if(endTime >= LocalTime)
		{
			return E_OK;
		}
	}
	else
	{
		if((startTime <= LocalTime)&&(LocalTime <= endTime))
		{
			return E_OK;

		}
		else if(LocalTime > endTime)
		{
			return E_TIME_OUT;
		}
	}
	return E_NOT_OK;
}

/****************************************************************************************************
**Function:
	Std_ReturnType Access_ComTimeLoop(uint32 startTime, uint32 endTime)
**Author: rory
**Description:
**Input: 
**Output: 
****************************************************************************************************/
Std_ReturnType Access_ComTimeLoop(uint32 startTime, uint32 endTime)
{
	uint32 currentTime;

	currentTime = BUILD_LOOP_TIME(ProtoAnaly_Rtcinfo.Hour, ProtoAnaly_Rtcinfo.Minute, ProtoAnaly_Rtcinfo.Second);
	startTime = startTime&0x1ffff;
	endTime = endTime&0x1ffff;
	if((startTime <= currentTime)&&(currentTime <= endTime))
	{
		return E_OK;
	}
	return E_NOT_OK;
}

#if 1//(SUPPORT_RECORD_LOC_STORE==STD_TRUE)
/****************************************************************************************************
**Function:
	void Access_WriteRecordFlash(uint8* pKeyId,uint32 time ,uint8 type, uint8 action)
**Author: rory
**Description:
**Input: 
**Output: 
****************************************************************************************************/
uint8  Access_GetRecord(uint16 index,uint8 *poutdat)
{
	poutdat[0] = BUILD_TYPE(AccRcord.RecordList[index].TypeResult);
	if((AccRcord.RecordList[index].TypeResult == ACCESS_LEARN_IDENTITY_CARD_TPYE)
		|| (AccRcord.RecordList[index].TypeResult == KEY_TYPE_IDENTITY_CARD))
	{
		poutdat[1] = 0xff;
		poutdat[2] = 0xff;
		poutdat[3] = 0xff;
		poutdat[4] = 0xff;
		poutdat[5] = TWOHI_UINT32(AccRcord.RecordList[index].Id);
		poutdat[6] = TWOLO_UINT32(AccRcord.RecordList[index].Id);
		poutdat[7] = ONEHI_UINT32(AccRcord.RecordList[index].Id);
		poutdat[8] = ONELO_UINT32(AccRcord.RecordList[index].Id);
	}
	else
	{
	poutdat[1] = TWOHI_UINT32(AccRcord.RecordList[index].Id);
	poutdat[2] = TWOLO_UINT32(AccRcord.RecordList[index].Id);
	poutdat[3] = ONEHI_UINT32(AccRcord.RecordList[index].Id);
	poutdat[4] = ONELO_UINT32(AccRcord.RecordList[index].Id);
	poutdat[5] = 0xff;
	poutdat[6] = 0xff;
	poutdat[7] = 0xff;
	poutdat[8] = 0xff;
	}
	poutdat[9] = BUILD_ACTION(AccRcord.RecordList[index].TypeResult);
	poutdat[10] = TWOHI_UINT32(AccRcord.RecordList[index].Time);
	poutdat[11] = TWOLO_UINT32(AccRcord.RecordList[index].Time);
	poutdat[12] = ONEHI_UINT32(AccRcord.RecordList[index].Time);
	poutdat[13] = ONELO_UINT32(AccRcord.RecordList[index].Time);
	return 1;
}
#endif
/****************************************************************************************************
**Function:
	void Access_WriteRecordFlash(uint8* pKeyId,uint32 time ,uint8 type, uint8 action)
**Author: rory
**Description:
**Input: 
**Output: 
****************************************************************************************************/
void Access_WriteRecordFlash(uint8* pKeyId,uint32 time ,uint8 type, uint8 action)
{
	if(AccRcord.Wrecordindex >= ACCESS_RECORD_MAX)
	{
		AccRcord.Wrecordindex = 0;
	}
	AccRcord.RecordList[AccRcord.Wrecordindex].Id = BUILD_UINT32(pKeyId[3], pKeyId[2], pKeyId[1], pKeyId[0]);
	AccRcord.RecordList[AccRcord.Wrecordindex].Time = time;
	AccRcord.RecordList[AccRcord.Wrecordindex].TypeResult = BUILD_ACTIONTYPE(type, action);
	AccRcord.Wrecordindex++;
	if(++AccRcord.recordnum >= 65535)
	{
		AccRcord.recordnum = 0;
	}
	RecordList_StorePara(0);
	Access_Record_Null = 0;
}
/****************************************************************************************************
**Function:
	Std_ReturnType Access_GetSupportType(uint8 idtype)
**Author: rory
**Description:
**Input: 
**Output: 
****************************************************************************************************/
Std_ReturnType Access_GetSupportType(uint8 idtype)
{
	switch(idtype)
	{
#if(SUPPORT_APP_ID == STD_TRUE)
	case KEY_TYPE_APP: 
	{
		
	}break; 
#endif
#if(SUPPORT_M1_CARD == STD_TRUE)
	case KEY_TYPE_CARD: 
	{
		
	}break; 	
#endif
#if(SUPPORT_QRCODE == STD_TRUE)
	case KEY_TYPE_QRCODE: 
	{
		
	}break; 	
#endif
#if(SUPPORT_PWSD == STD_TRUE)
	case KEY_TYPE_PASSWORD:
	{
	}break; 	
#endif
#if(SUPPORT_FINGERPRINT == STD_TRUE)
	case KEY_TYPE_FINGERPRINT:
	{
	}break; 
#endif
#if(SUPPORT_IDENTITY_CARD == STD_TRUE)
	case KEY_TYPE_IDENTITY_CARD:
	{
	}break;
#endif
	default:return E_NOT_OK;
	}
	return E_OK;
}
/****************************************************************************************************
**Function:
	void Access_EraseRecordData(void)
**Author: lxw
**Description:
**Input: 
**Output: 根据索引强制修改数据
****************************************************************************************************/
void Access_EraseRecordData(void)
{
	memset((uint8_t *)&AccRcord, 0xff, sizeof(AccRcordParaType));
	AccRcord.Wrecordindex = 0;
	AccRcord.Srecordindex = 0;
	AccRcord.recordnum = 0;
	AccRcord.TimerBk = BUILD_TIME(18,1,1,1,1,1);
	AccRcord.CustomTimerBk = 0;
	AccRcord.StaffTimerBk = 0;
	AccRcord.AllTimerBk = 0;
	AccRcord.EmergencyTimerBk = 0;
	AccRcord.AppTimerBk = 0;
	RecordList_StorePara(1);
}

/****************************************************************************************************
**Function:
	void Access_FlashArrang(void)
**Author: rory
**Description:
**Input: 
**Output: 
****************************************************************************************************/
void Access_FlashArrang(void)
{
	uint8 i,j;

	memset(&LockKeyStatistics,0,sizeof(LockKeyStatistics));
	for(i=0; i<BLACK_LIST_MAX; i++)
	{
		if(LockKeyBlackList.BlackList[i].KeyId != 0xffffffff)
		{
			LockKeyStatistics.BlackListCount++;
		}	
	}

	
	for ( j=0;j<SERVE_KEY_PAGEMAX;j++ )
	{
		ServeKeyList_Readflash( j );
	    for ( i=0; i<SERVE_KEY_MAX; i++)
	      {
			if (LockKeyServeKeyList.KeyList[i].KeyId != 0xffffffff)
			  {
				  LockKeyStatistics.ServKeyCount++;
			  }
		  }
	}
}
uint8 cardopenFlag;
/****************************************************************************************************
**Function:
	void Access_CardProcess(uint8 idtpye, uint8* pUid,Access_CardDatType *CardDat)
**Author: rory
**Description:
**Input: 
**Output: 
****************************************************************************************************/
void Access_CardProcess(uint8 idtpye, uint8* pUid,Access_CardDatType *CardDat)
{
	uint8 cardType;
	
	if(Sys_PataCfg.State == WORK_TEST)
	{
		Sys_PataCfg.State = WORK_NORMAL;
	}
	else if(Sys_PataCfg.State == WORK_LEARN_CARD)	
	{
		Sys_PataCfg.State = WORK_NORMAL;
		Access_CardLearnTimer64 = 0;
		Access_LearnCardOk(idtpye,pUid);
		Access_BeepStart(BEEP_NORMAL,1);
		Access_LightStart(LIGHT_OK,LIGHT_SLOW,1,0);
		if(idtpye == KEY_TYPE_IDENTITY_CARD)
		{
			cardType = ACCESS_LEARN_IDENTITY_CARD_TPYE;
		#if (defined(SUPPORT_RECORD_LOC_STORE)&&(SUPPORT_RECORD_LOC_STORE == STD_TRUE))
			Access_WriteRecordFlash(&pUid[4],Access_UserId_Temp, cardType, ACCESS_OPEN_LOCK_TPYE);
		#endif
		}
		else
		{
			cardType = ACCESS_LEARN_CARD_TPYE;
#if (defined(SUPPORT_RECORD_LOC_STORE)&&(SUPPORT_RECORD_LOC_STORE == STD_TRUE))
			Access_WriteRecordFlash(pUid,Access_UserId_Temp, cardType, ACCESS_OPEN_LOCK_TPYE);
#endif
		}
#if (SUPPORT_RECORD_RTIME_SEND == STD_TRUE)
		ProtoAnaly_AddRecordCmd(pUid,Access_UserId_Temp, cardType, ACCESS_OPEN_LOCK_TPYE);
#endif
        if(IS_CONNECTED())
		{
			Proto_UpdateLearnRecord(pUid,Access_UserId_Temp, cardType, ACCESS_OPEN_LOCK_TPYE);
		}
		return;
	}
	else if((Sys_PataCfg.State == WORK_NORMAL)
	#if (defined(CLASS_ROOM_ENABLE)&&(CLASS_ROOM_ENABLE == STD_TRUE))
		|| (Sys_PataCfg.State == WORK_NORMALLY_OPEN)
	#endif
		)
	{
		uint8 ret = E_NOT_OK;
		
		ret = LockKey_Check_CardKey(idtpye,pUid);
		if(ret == E_NOT_OK)
		{
#if (RC522_RDDAT == STD_TRUE)
				if(CardDat->cardtype == CARD_TYPE_GUEST)
				{
					if(CardDat->deviceId != Sys_PataCfg.Mac) 
					{
						ret = E_LOCK_ID_WRONG;
						goto ERR;
					}
					else if(CardDat->devicePaswd != Sys_PataCfg.Pswd) 
					{
						ret = E_USER_PASWD_WRONG;
						goto ERR;
					}
					else 
					{
					        idtpye = RECORD_CUSTOM_CARD;
						goto RIGHT;
					}
				}
				else if((CardDat->cardtype == CARD_TYPE_STAFF) || (CardDat->cardtype == CARD_TYPE_STAFF_LOOP))
				{
				  if(CardDat->cardtype == CARD_TYPE_STAFF)
					{
						idtpye = RECORD_STAFF_CARD;
					}
					else if(CardDat->cardtype == CARD_TYPE_STAFF_LOOP)
					{
						idtpye = RECORD_STAFF_LOOP_CARD;
					}
					goto RIGHT;
				}
				else if(CardDat->cardtype == CARD_TYPE_ALL)
				{
					idtpye = RECORD_TOTAL_CARD;	
					goto RIGHT;
				}
				else if(CardDat->cardtype == CARD_TYPE_EMERGENCY)
				{
					Sys_PataCfg.State = WORK_NORMALLY_OPEN;
					Access_Unlock();
					Access_LockTimer64 = 0;
					Access_BeepStart(BEEP_VERY_SLOW,1);
					idtpye = RECORD_EMERGENCY_CARD;
				#if (defined(SUPPORT_RECORD_LOC_STORE)&&(SUPPORT_RECORD_LOC_STORE == STD_TRUE))
					Access_WriteRecordFlash(pUid,ProtoAnaly_RtcLocalTime, idtpye, ACCESS_OPEN_LOCK_TPYE);
				#endif
				#if (SUPPORT_RECORD_RTIME_SEND == STD_TRUE)
					ProtoAnaly_AddRecordCmd(pUid,ProtoAnaly_RtcLocalTime, idtpye, ACCESS_OPEN_LOCK_TPYE);
				#endif
					return;
				}
				else if(CardDat->cardtype == CARD_TYPE_TESK)
				{
					goto RIGHT;
				}
				ret = E_KEY_NO_SUPPORT;
				goto ERR;
#endif			
		}
		else if(ret == E_OK)
		{
			goto RIGHT;
		}
	}
	else if(Sys_PataCfg.State == WORK_NORMALLY_OPEN)
	{
		uint8 ret = E_NOT_OK;
		
#if (RC522_RDDAT == STD_TRUE)
			if((CardDat->hotelId != Sys_PataCfg.PropertyId)||(CardDat->hotelId == 0))
			{
				ret = E_LOCK_ID_WRONG;
				goto ERR;
			}
			else 
			{
				if(CardDat->cardtype == CARD_TYPE_EMERGENCY)
				{
					ret = Access_ComTime(CardDat->Timest, CardDat->Timeend); 
					if(ret == E_OK)
					{
						//Access_Globle.State = WORK_NORMAL;
						Sys_PataCfg.State = WORK_NORMAL;
						goto RIGHT;
					}
				}
			}
#else
		Sys_PataCfg.State = WORK_NORMAL;
		//Access_Globle.State = WORK_NORMAL;
		goto RIGHT;	
#endif			
	}
	ERR:
	Access_OpenError();
	return ;
	RIGHT:
	cardopenFlag = 1;	
	Access_Unlock();
	uint8 action;
#if (defined(CLASS_ROOM_ENABLE)&&(CLASS_ROOM_ENABLE == STD_TRUE))
	if(Sys_PataCfg.State == WORK_NORMAL)
	{
		action = ACCESS_CLOSE_LOCK_TPYE;
	}
	else
	{
		action = ACCESS_OPEN_LOCK_TPYE;
	}
	Access_UpdateStatus();
#else
	action = ACCESS_OPEN_LOCK_TPYE;
#endif
#if (defined(SUPPORT_RECORD_LOC_STORE)&&(SUPPORT_RECORD_LOC_STORE == STD_TRUE))
	if(idtpye == KEY_TYPE_IDENTITY_CARD)
	{
		Access_WriteRecordFlash(&pUid[4],ProtoAnaly_RtcLocalTime, idtpye, ACCESS_OPEN_LOCK_TPYE);
	}
	else
	{
		Access_WriteRecordFlash(pUid,ProtoAnaly_RtcLocalTime,idtpye,ACCESS_OPEN_LOCK_TPYE);
	}
#endif
#if (SUPPORT_RECORD_RTIME_SEND == STD_TRUE)
		ProtoAnaly_AddRecordCmd(pUid,ProtoAnaly_RtcLocalTime,idtpye,ACCESS_OPEN_LOCK_TPYE);
#endif
}

/****************************************************************************************************
**Function:
	void Access_BeepStart(uint8 mode, uint8 times)
**Author: lxw
**Description:
**Input: 
**Output: 
****************************************************************************************************/
void Access_BeepStart(Beep_StateType mode, uint8 times)
{
	Beep_ComType Beep_Com;
	Beep_Com.state = mode; 
	Beep_Com.wkTimes = times;
	BeepDrive.write(&Beep_Com);
	//有   beep 时禁止读57卡
//	T5557Drive.release(NULL);		
}
/****************************************************************************************************
**Function:
	void Access_LightStart(uint8 index,uint8 mode, uint8 times,uint16 data)
**Author: lxw
**Description:
**Input: 
**Output: 
****************************************************************************************************/
void Access_LightStart(Light_NameType index,Light_StateType mode, uint8 times,uint16 data)
{
	Light_ComType Lightst;

	//Hc595_PowerOn();
	Lightst.lightname=index;
	Lightst.wkTimes = times;
	Lightst.state = mode;
	Lightst.Hc595dat = data;
	LightDrive.write(&Lightst);
	LightDrive.ioctl(LIGHT_CMD_SCAN,NULL);
}

/****************************************************************************************************
**Function:
	void Access_UpdateStatus(void)
**Author: lxw
**Description:
**Input: 
**Output: 
****************************************************************************************************/
void Access_UpdateStatus(void)
{
#if (SUPPORT_RECORD_RTIME_SEND == STD_TRUE)
	if(Sys_PataCfg.State == WORK_NORMAL)
	{
		Access_LockStatus |= DOOR_MOTOR_ST;
	}
	else
	{
		Access_LockStatus &= ~DOOR_MOTOR_ST;
	}
#endif
}

