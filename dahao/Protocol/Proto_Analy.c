/*****************************************************************************
|  File Name: ProtoAnaly.c
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
|-----------------------------------------------------------------------------
|               R E V I S I O N   H I S T O R Y
|-----------------------------------------------------------------------------
| Date        Ver     Author  Description
| ---------   ----    ------  ------------------------------------------------
| 2013-11 1.00    lxw     Initial version 
|****************************************************************************/
#include"protocol\Proto_Analy.h"
#include"Protocol\factInfo.h"
#include"Protocol\Proto_ComDef.h"
#include "SYSTEM\sys.h"
#include"SYSTEM\Sys_GenFun.h"
#include "Protocol\Proto_CommPack.h"
//#include "UART\UART.h"
//#include "KEY\key.h"
//#include "WatchDog\WatchDog.h"
//#include "Flash\Flash.h"
#include "RTC\Rtc.h"
#include "Access\Access.h"
#include "Protocol\Proto_NetComm.h"
#include "Beep\Beep.h"
#include "Light\light.h"
#include <cstdlib>
#include "ble_dahao.h"
//#include "Flash\Flash.h"
#include "Access\LockKey.h"
#include "Encpt\Encpt.h"
#include "FACTORY\Factory.h"
#include "CustomCode\CustomCode.h"
#include "crc32.h"
//#include "lora\sx1276-LoRa.h"
#include "MOTOR\motor.h"

uint8  protoAnaly_netCommType;
uint32 ProtoAnaly_RtcLocalTime;
uint8  ProtoAnaly_GetAckTimes;
uint8  ProtoAnaly_EthTryTimes;

uint16 ProtoAnaly_HeartTimeOutx64ms;


Proto_NetcommType Proto_NetcommBack;
Rtc_Type ProtoAnaly_Rtcinfo;
extern	Proto_NetcommType Proto_Netcomm;
extern  uint8  Lora_Sync_Flag;
extern  uint32 Sys_WkTick ;

extern void ProtoAnaly_LoraSlaveCallBackProc(void);
//extern void Lora_timer_start(uint8 sttype,uint16 time);

uint8  ProtoAnaly_LoraBrgBuf[30];
uint8  ProtoAnaly_LoraBrgSize;
uint8  ProtoAnalyRecordBuf[PROTOANALY_RECORD_CACHEMAX][16];
uint8  ProtoAnalyRecordIndex =0;
uint8  ProtoAnalyRecordSendErr =0;
uint32 ProtoAnalyHashKey;
uint8  ProtoAnaly_LoraSendTims;
uint8  protoAnaly_freqsetflag;
int16  ProtoAnaly_LoraSsi = 0;


/*
Sleep: 0:
Hread: 
*/
const uint32 PROTO_SLEEP_TIME_TABLE[]=
{
	PROTOANALY_REC_WINDOWTIMES*(PROTOANALY_CYCLETIMES_MAX -9),
	PROTOANALY_REC_WINDOWTIMES*(PROTOANALY_CYCLETIMES_MAX -(2*8+1)),
	PROTOANALY_REC_WINDOWTIMES*(PROTOANALY_CYCLETIMES_MAX -(4*8+1)),
};

const uint8 PROTO_BREAD_TIME_TABLE[]=
{
8,8,28,
};

/****************************************************************************************************
**Function:
	void ProtoAnaly_EthAckOk(Proto_NetcommType CommType)
**Author: lxw
**Description:
**Input: 
**Output: 
****************************************************************************************************/
void ProtoAnaly_EthAckOk(Proto_NetcommType CommType)
{
	protoAnaly_netCommType = CommType;
}
/****************************************************************************************************
**Function:
	void ProtoAnaly_UpdateTime(void)
**Author: rory
**Description:
**Input: 
**Output: 
****************************************************************************************************/
void ProtoAnaly_UpdateTime(void)
{
	Rtc_Type Rtc_data;

	if(Rtc_Read(&Rtc_data)!= E_OK)
	{
		return ;
	}
	if((Rtc_data.Year > 99 )||(Rtc_data.Month > 12)||(Rtc_data.Day> 31)
		||(Rtc_data.Hour> 23)||(Rtc_data.Minute> 60)||(Rtc_data.Second> 60))
	{
		/*Rtc_data.Year = 0;
		memset(&Rtc_data,0,sizeof(Rtc_data));
		RtcDrive.open(NULL);*/
		return ;
	}
	memcpy(&ProtoAnaly_Rtcinfo,&Rtc_data,sizeof(ProtoAnaly_Rtcinfo));
	ProtoAnaly_RtcLocalTime = (((uint32)ProtoAnaly_Rtcinfo.Year << 26) & 0xfc000000) + (((uint32)ProtoAnaly_Rtcinfo.Month << 22) & 0x3c00000)
	+ (((uint32)ProtoAnaly_Rtcinfo.Day << 17) & 0x3e0000) + (((uint32)ProtoAnaly_Rtcinfo.Hour << 12) & 0x1f000)
	+ (((uint32)ProtoAnaly_Rtcinfo.Minute << 6) & 0xfc0) + (ProtoAnaly_Rtcinfo.Second & 0x3f);
	AccRcord.TimerBk = ProtoAnaly_RtcLocalTime;
	ProtoAnaly_Rtcinfo.Week = Main_GetWeekFromDay(ProtoAnaly_Rtcinfo.Year,ProtoAnaly_Rtcinfo.Month,ProtoAnaly_Rtcinfo.Day);
 }


 /****************************************************************************************************
 **Function:
	 void ProtoAnaly_UpdateTime(void)
 **Author: rory
 **Description:
 **Input: 
 **Output: 
 ****************************************************************************************************/
 void ProtoAnaly_LoraSetTime(uint32 rtime)
 {
	 Rtc_Type Rtc_data;

	 Rtc_data.Year = ((rtime& 0xfc000000) >> 26) ;
	 Rtc_data.Month = ((rtime& 0x3c00000) >> 22) ;
	 Rtc_data.Day = ((rtime& 0x3e0000) >> 17) ;
	 Rtc_data.Hour = ((rtime& 0x1f000) >> 12) ;
	 Rtc_data.Minute = ((rtime& 0xfc0) >> 6) ;
	 Rtc_data.Second = (rtime& 0x3f) ;
	 Rtc_Ioctl(RTC_CLOCKSET,&Rtc_data);
  }

 /****************************************************************************************************
 **Function:
	 void ProtoAnaly_HashKeyInit(void)
 **Author: lxw
 **Description:  
 **Input: 
 **Output: 
 ****************************************************************************************************/
 uint32 ProtoAnaly_HashKeyInit(uint32 paswd)
{
	 uint8 hashbuf[8];
	srand(Sys_PataCfg.Mac);
	
	Sys_GenFun32To8(paswd,hashbuf);
	Sys_GenFun32To8(Sys_PataCfg.Mac,&hashbuf[4]);
//	ProtoAnalyHashKey= BKDRHash(hashbuf,8);
	return BKDRHash(hashbuf,8);
}

 /****************************************************************************************************
 **Function:
	 void ProtoAnaly_Init(void)
 **Author: lxw
 **Description:  
 **Input: 
 **Output: 
 ****************************************************************************************************/
 void ProtoAnaly_Init(void)
{
	uint8 hashbuf[8];
	 Proto_PackfunCallBack Proto_Pack;
#if (defined LORA_ENABLE) && (LORA_ENABLE == STD_TRUE)
	Sys_PataCfg.rfPara.ConnectCallBack = NULL;
	Sys_PataCfg.rfPara.ProcCallBack=Proto_CommHostUnPackRec;
	RadioDrive.open(&Sys_PataCfg.rfPara);
#endif


/*	Proto_Pack.Tc_TargeAdd	= NET_NULL;
	Proto_Pack.Tc_SourceAdd1 = NET_NULL;
	Proto_Pack.Tc_SourceAdd2 = NET_NULL;
*/
	Proto_Pack.AppProcCallBack = ProtoAnaly_CmdAppProc;
	Proto_Pack.MacProcCallBack = ProtoAnaly_CmdMacProc;
	Proto_Pack.ProcEndCallBack = ProtoAnaly_ProcEnd;
	Proto_Pack.ProcNetUpdateCallBack = ProtoAnaly_EthAckOk;
	Proto_Pack.PublicProcCallBack = ProtoAnaly_PublicProc;
	Proto_CommPrackInit(&Proto_Pack);
	ProtoAnalyHashKey = ProtoAnaly_HashKeyInit(Sys_PataCfg.Pswd);
	protoAnaly_freqsetflag = 0;
	Sys_SleepErrTimsoutx64ms = 0;
}
 /****************************************************************************************************
 **Function:
 void ProtoAnaly_SendHeartBeat(uint8 cmd,uint8 ch,uint8 state)
 **Author: lxw
 **Description:  
 **Input: 
 **Output: 
 ****************************************************************************************************/
 void ProtoAnaly_SendHeartBeat(uint8 cmd,uint8 ch,uint8 state)
 {
	uint8 datbuf[30]={0};
	datbuf[0] = 4;
	datbuf[1] = COMD_GET_HEARTBEAT;
	datbuf[8] = state;
	datbuf[9] = TYPE_LOCK_ACCESS_REMOTE;
	datbuf[10] = 0;
	Proto_CommSend(LORA,datbuf);
 }
/****************************************************************************************************
**Function:
	void ProtoAnaly_CleanErr(void)
**Author: rory
**Description:
**Input: 
**Output: 
****************************************************************************************************/
uint16	ProtoAnaly_GprsHeartTimeOutx16ms;
uint16 	ProtoAnaly_GprsHeartErr;
uint8 	ProtoAnaly_GprsRstHeartTimes= 0;

/****************************************************************************************************
**Function:
	Std_ReturnType ProtoAnaly_GetHeartbeatIn(uint32 id)
**Author: rory
**Description:
**Input: 
**Output: 
****************************************************************************************************/
Std_ReturnType ProtoAnaly_RemoveId(uint8 idtpye,uint8* id)
{	
	uint8 year, month, day, i;
	uint32 tempTime;
	
	uint32 keyId;
	
	if(idtpye == KEY_TYPE_IDENTITY_CARD)
	{
		keyId = BUILD_UINT32(id[7],id[6],id[5],id[4]);
	}
	else
	{
		keyId = BUILD_UINT32(id[3],id[2],id[1],id[0]);
	}
	
#if (defined RTC_EN) && (RTC_EN == STD_TRUE)
	ProtoAnaly_UpdateTime();
#endif
	
	if((keyId == 0xffffffff) || (keyId == 0))
	{
		return E_OK;
	}
	else
	{
		for (uint8 j=0;j<SERVE_KEY_PAGEMAX;j++ )
		{
	    	ServeKeyList_Readflash( j );	
			for(i=0; i<SERVE_KEY_MAX; i++)
			{
				if(idtpye == LockKeyServeKeyList.KeyList[i].Type)
				{
					if(LockKeyServeKeyList.KeyList[i].KeyId == keyId)
					{
						if((idtpye == KEY_TYPE_ONE_TIME) || (idtpye == KEY_TYPE_FOREVER)
						|| (idtpye == KEY_TYPE_TIME_SLOT_7) || (idtpye == KEY_TYPE_CLEAR)
						|| (idtpye == KEY_TYPE_TIME_SLOT_8))
						{
							tempTime = LockKeyServeKeyList.KeyList[i].StartTime;
							year = BUILD_YEAR(tempTime);
							month = BUILD_MONTH(tempTime);
							day = BUILD_DAY(tempTime);
							if((ProtoAnaly_Rtcinfo.Year == year)&&(ProtoAnaly_Rtcinfo.Month == month)
							&&(ProtoAnaly_Rtcinfo.Day == day))
							{ /* 加入黑名单*/
								BlackList_Add(LockKeyServeKeyList.KeyList[i].Type, LockKeyServeKeyList.KeyList[i].KeyId,
								BUILD_TIME(ProtoAnaly_Rtcinfo.Year,ProtoAnaly_Rtcinfo.Month,ProtoAnaly_Rtcinfo.Day,
								23,59,59));
								BlackList_StorePara();
							}
						}
						memset(&LockKeyServeKeyList.KeyList[i], 0xff, sizeof(LockKey_ServeKeyType));
						if(LockKeyStatistics.ServKeyCount>0)
						{
							LockKeyStatistics.ServKeyCount--;
						}
						
						ServeKeyList_Writeflash( j );
						//ServeKeyList_StorePara1();//
						return E_OK;
			   		}
		    	}	
			}
		}
		if((idtpye == KEY_TYPE_ONE_TIME) || (idtpye == KEY_TYPE_FOREVER)
		||(idtpye == KEY_TYPE_CLEAR) || (idtpye == KEY_TYPE_TIME_SLOT_8))
						{
			 /* 加入黑名单*/
			BlackList_Add(idtpye, keyId,
			BUILD_TIME(ProtoAnaly_Rtcinfo.Year,ProtoAnaly_Rtcinfo.Month,ProtoAnaly_Rtcinfo.Day,
				23,59,59));
			BlackList_StorePara();
			}
		else if(idtpye == KEY_TYPE_TIME_SLOT_7)
			{
		/* 加入黑名单*/
			BlackList_Add(idtpye, keyId,
			BUILD_TIME(ProtoAnaly_Rtcinfo.Year,ProtoAnaly_Rtcinfo.Month,(ProtoAnaly_Rtcinfo.Day+1),
		23,59,59));
		BlackList_StorePara();
		}
	}
	return E_OK;
}
/****************************************************************************************************
**Function:
	void ProtoAnaly_AddId(uint8 idtpye,uint32 id)
**Author: rory
**Description:
**Input: 
**Output: 
****************************************************************************************************/
Std_ReturnType ProtoAnaly_AddId(uint8 idtpye,uint8 *idbuf,uint8 num)
{
	uint16 index = 0xff;
	uint8  flashoffset;
	
	uint8 type;
	uint8 err = E_NOT_OK;
	uint32 keyId;

		keyId = BUILD_UINT32(idbuf[3], idbuf[2], idbuf[1], idbuf[0]);
	BlackList_RemoveKey(idtpye, keyId);
	for (uint8 j=0;j<SERVE_KEY_PAGEMAX;j++ )
	{
	    ServeKeyList_Readflash( j );	
		for(uint8 i=0; i<SERVE_KEY_MAX; i++)
		{
			if(keyId == LockKeyServeKeyList.KeyList[i].KeyId)
			{//强制修改有效时间
				LockKeyServeKeyList.KeyList[i].EndTime = BUILD_UINT32(idbuf[7], idbuf[6], idbuf[5], idbuf[4]);
				LockKeyServeKeyList.KeyList[i].StartTime = BUILD_UINT32(idbuf[11], idbuf[10], idbuf[9], idbuf[8]);
				LockKeyServeKeyList.KeyList[i].Cycle = idbuf[12];
				
				ServeKeyList_Writeflash( j );
				//ServeKeyList_StorePara1();//
				return E_OK;
			}
			if(index == 0xff)
			{
				if(LockKeyServeKeyList.KeyList[i].KeyId == 0xffffffff)
				{
				    flashoffset = j;
					index = i;
				}
			}
		}
	}
	if(index != 0xff)
	{//增新
	    ServeKeyList_Readflash( flashoffset );
		LockKeyServeKeyList.KeyList[index].Type = idtpye;
		LockKeyServeKeyList.KeyList[index].KeyId = keyId;
		LockKeyServeKeyList.KeyList[index].EndTime = BUILD_UINT32(idbuf[7], idbuf[6], idbuf[5], idbuf[4]);
		LockKeyServeKeyList.KeyList[index].StartTime = BUILD_UINT32(idbuf[11], idbuf[10], idbuf[9], idbuf[8]);
		LockKeyServeKeyList.KeyList[index].Cycle = idbuf[12];
		LockKeyStatistics.ServKeyCount++;
		ServeKeyList_Writeflash( flashoffset );
		//ServeKeyList_StorePara1();
		return E_OK;
	}

	for (uint8 j=0;j<SERVE_KEY_PAGEMAX;j++ )
	{
	    ServeKeyList_Readflash( j );	
		for(uint8 i=0; i<SERVE_KEY_MAX; i++)
		{
			type = LockKeyServeKeyList.KeyList[i].Type;
			if(((type == KEY_TYPE_ONE_TIME) || (type == KEY_TYPE_TIME_SLOT_7) 
				|| (type == KEY_TYPE_TIME_SLOT_8)) && (LockKeyServeKeyList.KeyList[i].KeyId != 0xffffffff))
			{
				LockKeyServeKeyList.KeyList[i].Type = idtpye;
				LockKeyServeKeyList.KeyList[i].KeyId = keyId;
				LockKeyServeKeyList.KeyList[i].EndTime = BUILD_UINT32(idbuf[7], idbuf[6], idbuf[5], idbuf[4]);
				LockKeyServeKeyList.KeyList[i].StartTime = BUILD_UINT32(idbuf[11], idbuf[10], idbuf[9], idbuf[8]);
				LockKeyServeKeyList.KeyList[i].Cycle = idbuf[12];
				
				ServeKeyList_Writeflash( j );
				//ServeKeyList_StorePara1();//
				return E_OK;
			}
		}
	}
	return err;
}

/****************************************************************************************************
**Function:
	void ProtoAnaly_SetTimeIn(uint8_t *pInData ,uint8_t *pOutData)
**Author: rory
**Description:
**Input: 
**Output: 
****************************************************************************************************/
void ProtoAnaly_SetTimeIn(uint8_t *pInData ,uint8_t *pOutData)
{
	Rtc_Type Rtc_data;

	pOutData[0] = 2; 			/* 长度*/
	pOutData += 7;
	Rtc_data.Year = pInData[5];
	Rtc_data.Month= pInData[6];
	Rtc_data.Day= pInData[7];
	Rtc_data.Hour= pInData[8];
	Rtc_data.Minute= pInData[9];
	Rtc_data.Second= pInData[10];
	AccRcord.AppTimerBk = 0;
	Rtc_Ioctl(RTC_CLOCKSET,&Rtc_data);
	pOutData[1] = E_OK;
	Access_BeepStart(BEEP_NORMAL,1);
}
/****************************************************************************************************
**Function:
	void ProtoAnaly_GetTimeIn(uint8_t *pInData ,uint8_t *pOutData)
**Author: rory
**Description:
**Input: 
**Output: 
****************************************************************************************************/
void ProtoAnaly_GetTimeIn(uint8_t *pInData ,uint8_t *pOutData)
{
	pOutData[0] = 8; /* 长度*/
	pOutData += 7;
#if (defined RTC_EN) && (RTC_EN == STD_TRUE)
	ProtoAnaly_UpdateTime();
#endif

	pOutData[1]= E_OK;
	pOutData[2]= ProtoAnaly_Rtcinfo.Year;
	pOutData[3] = ProtoAnaly_Rtcinfo.Month;
	pOutData[4] = ProtoAnaly_Rtcinfo.Day;
	pOutData[5]= ProtoAnaly_Rtcinfo.Hour;
	pOutData[6] =ProtoAnaly_Rtcinfo.Minute;
	pOutData[7]= ProtoAnaly_Rtcinfo.Second;
	Access_BeepStart(BEEP_NORMAL,1);
}
/****************************************************************************************************
**Function:
	void ProtoAnaly_ConfigHostIn(uint8_t *pInData ,uint8_t *pOutData)
**Author: rory
**Description:
**Input: 
**Output: 
****************************************************************************************************/
void ProtoAnaly_ConfigHostIn(uint8_t *pInData ,uint8_t *pOutData)
{
	uint16 heart;
	pOutData[0] = 2; 			/* 长度*/
	pOutData += 7;
	heart = BUILD_UINT16(pInData[7], pInData[6]);
	if((heart > 60*60) || (heart < 10))
	{
		return;
	}
	//Sys_PataCfg.UploadRecord = pInData[5];
	Sys_PataCfg.HeartTime = heart;
	pOutData[1] = E_OK;
	Access_BeepStart(BEEP_NORMAL,1);
}
/****************************************************************************************************
**Function:
	void ProtoAnaly_ReaderUploadId(uint8_t *pInData ,uint8_t *pOutData)
**Author: rory
**Description:
**Input: 
**Output: 
****************************************************************************************************/
void ProtoAnaly_ReaderUploadCardId(uint8* pCardId, uint8 type)
{
	uint8 Databuffer[30];
//	uint8 index = 0;
	*((uint16*)Databuffer)= Sys_GenFunhtons(PROTO_COMMPACK_HOST_DAT_OUT);
	Databuffer[3] = COMD_READER_UNLOAD_CARD_ID; // command
	*((uint16*)(&Databuffer[4]))= Sys_GenFunhtons(Sys_PataCfg.SysId);
	*((uint32*)(&Databuffer[6]))= Sys_GenFunhtonl(Sys_PataCfg.Mac);
	Databuffer[10] = E_OK; // status
	Databuffer[11] = type; // Type

	if(type == KEY_TYPE_CARD)
	{
		Databuffer[2] = 14;  // length;
		Databuffer[12] = pCardId[0];
		Databuffer[13] = pCardId[1];
		Databuffer[14] = pCardId[2];
		Databuffer[15] = pCardId[3];
		Databuffer[16]  = Sys_GenFunChecksum(&Databuffer[2]);
	}
	else
	{
		Databuffer[2] = 18;  // length;
		Databuffer[12] = pCardId[0];
		Databuffer[13] = pCardId[1];
		Databuffer[14] = pCardId[2];
		Databuffer[15] = pCardId[3];
		Databuffer[16] = pCardId[0];
		Databuffer[17] = pCardId[1];
		Databuffer[18] = pCardId[2];
		Databuffer[19] = pCardId[3];
		Databuffer[20]  = Sys_GenFunChecksum(&Databuffer[2]);
	}
}

/****************************************************************************************************
**Function:
	void ProtoAnaly_ConfigDeviceExt(uint8_t *pInData)
**Author: rory
**Description:
**Input: 
**Output: 
****************************************************************************************************/
void ProtoAnaly_ConfigDeviceExtIn(uint8_t *pInData, uint8_t *pOutData)
{
	uint16 temp;
	uint8 ret;
	
	pOutData[0] = 2; 			/* 长度*/
	pOutData += 7;
	pOutData[1] = E_OK;

	if(pInData[5] == CONFIG_KEY_AUTOLOCK)
	{
		if(pInData[6] == 1)
		{
			Sys_PataCfg.CfgFlag |= AUTOLOCK;
			Sys_PataCfg.OpenLockTime = BUILD_UINT16(pInData[8], pInData[7]);
		}
		else
		{
			Sys_PataCfg.CfgFlag &= ~AUTOLOCK;
		}
	}
	else if(pInData[5] == CONFIG_KEY_BTSAVEMODE)
	{
		if(pInData[6] == 1)
		{
			Sys_PataCfg.CfgFlag |= BTSAVEMODE;
		}
		else
		{
			Sys_PataCfg.CfgFlag &= ~BTSAVEMODE;
		}
	}
	else if(pInData[5] == CONFIG_KEY_DETCARDSLEEP)
	{
		if(pInData[6] == 1)
		{
			Sys_PataCfg.CfgFlag |= DETCARDSLEEP;
		}
		else
		{
			Sys_PataCfg.CfgFlag &= ~DETCARDSLEEP;
		}
	}
	else if(pInData[5] == CONFIG_KEY_DISIBLEDOORLOCK)
	{
		if(pInData[6] == 1)
		{
			Sys_PataCfg.CfgFlag |= DISIBLEDOORLOCK;
		}
		else
		{
			Sys_PataCfg.CfgFlag &= ~DISIBLEDOORLOCK;
			if(DoorIsOpen())
			{
				Motor_DoorLockSt = MOTOR_DOORLOCK_OPEN;
			}
			else
			{
				Motor_DoorLockSt = MOTOR_DOORLOCK_OPENLOCK;
			}
		}
	}
	
	Sys_StoreFlag = STD_TRUE;
	Sys_McuRestFlag = STD_TRUE;
	Access_BeepStart(BEEP_FAST,1);
}


/****************************************************************************************************
**Function:
	void ProtoAnaly_ReadDeviceInfoExtIn(uint8_t *pInData ,uint8_t *pOutData)
**Author: rory
**Description:
**Input: 
**Output: 
****************************************************************************************************/
void ProtoAnaly_ReadDeviceInfoExtIn(uint8_t *pInData ,uint8_t *pOutData)
{
	pOutData[0] = 2+2; /* 长度*/
	pOutData += 7;
	pOutData[1]= E_OK;
	pOutData[2] = pInData[5];
	pOutData[3] = 0;

	if(pInData[5] == CONFIG_KEY_AUTOLOCK)
	{
		if(Sys_PataCfg.CfgFlag&AUTOLOCK)
		{
			pOutData[3] = 1;
			pOutData[4] = HI_UINT16(Sys_PataCfg.OpenLockTime);
			pOutData[5] = LO_UINT16(Sys_PataCfg.OpenLockTime);
			pOutData -= 7;
			pOutData[0] += 2;
		}
	}
	else if(pInData[5] == CONFIG_KEY_BTSAVEMODE)
	{
		if(Sys_PataCfg.CfgFlag&BTSAVEMODE)
		{
			pOutData[3] = 1;
		}
	}
	else if(pInData[5] == CONFIG_KEY_DETCARDSLEEP)
	{
		if(Sys_PataCfg.CfgFlag&DETCARDSLEEP)
		{
			pOutData[3] = 1;
		}
	}
	else if(pInData[5] == CONFIG_KEY_DISIBLEDOORLOCK)
	{
		if(Sys_PataCfg.CfgFlag&DISIBLEDOORLOCK)
		{
			pOutData[3] = 1;
		}
	}
	Access_BeepStart(BEEP_FAST,1);
}

/****************************************************************************************************
**Function:
	void ProtoAnaly_ModifyPaswdIn(uint8_t *pInData ,uint8_t *pOutData)
**Author: rory
**Description:
**Input: 
**Output: 
****************************************************************************************************/
void ProtoAnaly_ModifyPaswdIn(uint8_t *pInData ,uint8_t *pOutData)
{
	pOutData[0] = 2; 			/* 长度*/
	pOutData += 7;

       Sys_PataCfg.Pswd = BUILD_UINT32(pInData[8], pInData[7], pInData[6], pInData[5]);
	pOutData[1] = E_OK;
	Sys_StorePara();
	Access_BeepStart(BEEP_FAST,1);
}
/****************************************************************************************************
**Function:
	uint8 ProtoAnaly_CheckTime(uint8_t *pInData)
**Author: rory
**Description:
**Input: 
**Output: 
****************************************************************************************************/
uint8 ProtoAnaly_CheckTime(uint8_t *pInData)
{
	Rtc_Type Rtc_data;
	uint8 ret = E_OK;
	
	ProtoAnaly_UpdateTime();
	
	uint32 MobileTime = BUILD_UINT32(pInData[3], pInData[2], pInData[1], pInData[0]);
	if((MobileTime == 0xffffffff) || (MobileTime == 0))
	{
		return ret;
	}
#if 1	
	if(AccRcord.AppTimerBk < MobileTime)    //   sheva
	{
		AccRcord.AppTimerBk = MobileTime;
	}
	else
	{
	/* 无效开锁*/
		return E_TIME_OUT;
	}
#endif
	Rtc_data.Year = (MobileTime &0xfc000000) >> 26;
	Rtc_data.Month = (MobileTime &0x3c00000) >> 22;
	Rtc_data.Day = (MobileTime &0x3e0000) >> 17;
	Rtc_data.Hour = (MobileTime &0x1f000) >> 12;
	Rtc_data.Minute = (MobileTime &0xfc0) >> 6;
	Rtc_data.Second = (MobileTime &0x3f);

	if((Rtc_data.Year < 18) || (Rtc_data.Month > 12) || (Rtc_data.Hour > 24)
	    || (Rtc_data.Minute > 60) || (Rtc_data.Second > 60))
	{
		return ret;
	}
	if(ProtoAnaly_Rtcinfo.Year < 18)
	{
		Rtc_Ioctl(RTC_CLOCKSET,&Rtc_data);
	}
	else
	{
		uint32 tempTime;
		if(MobileTime > ProtoAnaly_RtcLocalTime)
		{
			tempTime = MobileTime - ProtoAnaly_RtcLocalTime;
		}
		else
		{
			tempTime = ProtoAnaly_RtcLocalTime - MobileTime;
		}
		if(tempTime > 512)
		{
			Rtc_Ioctl(RTC_CLOCKSET, &Rtc_data);
			return ret;
		}
	}
	return ret;
}
uint8 initFlag;

/****************************************************************************************************
**Function:
	void ProtoAnaly_GetTimeIn(uint8_t *pInData ,uint8_t *pOutData)
**Author: rory
**Description:
**Input: 
**Output: 
****************************************************************************************************/
void ProtoAnaly_PowerOnIn(uint8 *pInData ,uint8 *pOutData)
{
	uint8 ret = E_OK;
	
	pOutData[0] = 3; /* 长度*/
	pOutData += 7;
#if (defined CLASS_ROOM_ENABLE) && (CLASS_ROOM_ENABLE == STD_FALSE)
	if(Sys_PataCfg.State == WORK_NORMALLY_OPEN)
	{
		Access_BeepStart(BEEP_LOW,BEEP_ERROR_EMERGENCY);
		pOutData[1]= ret;
		pOutData[2]= Access_BatteryData;
		return;
	}
#endif
	ret = ProtoAnaly_CheckTime(&pInData[5]);
	if(ret == E_OK)
	{
	    if ( initFlag )
	      {
	         initFlag = 0;
	      }
		else
		  {
			Access_Unlock();
		  }
	}
	else
	{
		Access_LightStart(LIGHT_NG,LIGHT_SLOW,1,0);
		Access_BeepStart(BEEP_FAST,BEEP_ERROR_TIME_OUT);
	}
	pOutData[1]= ret;
	pOutData[2]= Access_BatteryData;
}
/****************************************************************************************************
**Function:
	void ProtoAnaly_GetTimeIn(uint8_t *pInData ,uint8_t *pOutData)
**Author: rory
**Description:
**Input: 
**Output: 
****************************************************************************************************/
void ProtoAnaly_PowerOffIn(uint8 *pInData ,uint8 *pOutData)
{
	pOutData[0] = 3; /* 长度*/
	pOutData += 7;
	pOutData[1]= E_OK ;
        pOutData[2]= Access_BatteryData;
	Access_Lock();
}

uint8 tbuf[20];
/****************************************************************************************************
**Function:
	void ProtoAnaly_SetListIn(uint8_t *pInData ,uint8_t *pOutData)
**Author: rory
**Description:
**Input: 
**Output: 
****************************************************************************************************/
void ProtoAnaly_SetListIn(uint8 *pInData ,uint8 *pOutData)
{
	pOutData[0] = 4;			/* 长度*/
	pOutData +=7;
	memcpy(tbuf,pInData,20);
	pInData +=4;
	if((pInData[1] == KEY_TYPE_ONE_TIME) || (pInData[1] == KEY_TYPE_FOREVER)
			|| (pInData[1] == KEY_TYPE_TIME_SLOT_7) || (pInData[1] == KEY_TYPE_CLEAR)
			|| (pInData[1] == KEY_TYPE_TIME_SLOT_8))
	{
      return;
	}
	pOutData[1] = ProtoAnaly_AddId(pInData[1],&pInData[3],pInData[2]);
	/*if(pInData[1] == KEY_TYPE_IDENTITY_CARD)
	{
		pOutData[2]=(pInData[2+pInData[2]*12]);
		pOutData[3]=(pInData[3+pInData[2]*12]);
	}
	else */
	{
	pOutData[2]=(pInData[3+(pInData[2]<<3)]);
	pOutData[3]=(pInData[4+(pInData[2]<<3)]);
	}
	if(pOutData[1] == E_OK)
	{
		Access_BeepStart(BEEP_NORMAL,1);
	}
}
/****************************************************************************************************
**Function:
	void ProtoAnaly_SetListIn(uint8_t *pInData ,uint8_t *pOutData)
**Author: rory
**Description:
**Input: 
**Output: 
****************************************************************************************************/
void ProtoAnaly_RomveIdin(uint8 *pInData ,uint8 *pOutData)
{
	pOutData[0] = 4;			/* 长度*/
	pOutData += 7;	
	pInData +=4;

#if (defined DEBUG_ENABLE) && (DEBUG_ENABLE == STD_TRUE)
	Sys_Debug(STD_OFF);
#endif
	pOutData[1] = ProtoAnaly_RemoveId(pInData[1],&pInData[2]);
#if (defined DEBUG_ENABLE) && (DEBUG_ENABLE == STD_TRUE)
	Sys_Debug(STD_ON);
#endif
	/*if(pInData[1] == KEY_TYPE_IDENTITY_CARD)
	{
		pOutData[2]=(pInData[2+12]);
		pOutData[3]=(pInData[3+12]);
	}
	else */
	{
		pOutData[2]=(pInData[2+(1<<3)]);
		pOutData[3]=(pInData[3+(1<<3)]);
	}
	
	if(pOutData[1] == E_OK)
	{
		Access_BeepStart(BEEP_NORMAL,1);
	}
}

/****************************************************************************************************
**Function:
	void ProtoAnaly_SetTimeIn(uint8_t *pInData ,uint8_t *pOutData)
**Author: rory
**Description:
**Input: 
**Output: 
****************************************************************************************************/
void ProtoAnaly_SetParaDefIn(uint8_t *pInData ,uint8_t *pOutData)
{	
	pOutData[0] = 2;	/* 长度*/
	pOutData += 7;
	pOutData[1] = E_OK;

	Sys_Parainit();
	Sys_StorePara();
	Access_BeepStart(BEEP_FAST,3);

// 等启动时自动复位所有参数 
}
/****************************************************************************************************
**Function:
	void ProtoAnaly_ReadKeyInfo(uint8_t *pInData ,uint8_t *pOutData)
**Author: rory
**Description:
**Input: 
**Output: 
****************************************************************************************************/
void ProtoAnaly_ReadKeyInfo(uint8_t *pInData ,uint8_t *pOutData)
{
	pOutData[0] = 9+15;			/* 长度*/
	pOutData += 7;
	pOutData[1]  = E_OK;//LockKey_Read_Key(&pInData[5], &pOutData[2]);
	Access_BeepStart(BEEP_NORMAL,1);
}

/****************************************************************************************************
**Function:
	void ProtoAnaly_AddKeyInfo(uint8_t *pInData ,uint8_t *pOutData)
**Author: rory
**Description:
**Input: 
**Output: 
****************************************************************************************************/
void ProtoAnaly_AddKeyInfo(uint8_t *pInData ,uint8_t *pOutData)
{
	pOutData[0] = 2;			/* 长度*/
	pOutData += 7;
	pOutData[1] =  E_OK;

#if (defined DEBUG_ENABLE) && (DEBUG_ENABLE == STD_TRUE)
	Sys_Debug(STD_OFF);
#endif
	if(Access_GetSupportType(pInData[5]) == E_NOT_OK)
	{
		pOutData[1] =  E_NOT_OK;
		return;
	}
	
	if(LockKeyStatistics.ServKeyCount > 1500)    //  SERVE_KEY_MAX
	{
		pOutData[1]= E_NOT_OK;
		return;
	}
	if(pInData[5] == KEY_TYPE_PASSWORD)
	{
		pOutData[1] = ProtoAnaly_AddId(KEY_TYPE_PASSWORD, &pInData[10], 1);
	}
	else if((pInData[5] == KEY_TYPE_CARD)||(pInData[5] == KEY_TYPE_IDENTITY_CARD))
	{
		pOutData[1] = LockKey_Start_LearnCardKey(&pInData[6]);
	}
	if(pOutData[1] == E_OK)
	{
		Access_BeepStart(BEEP_NORMAL,1);
	}
	else
	{
		Access_BeepStart(BEEP_FAST,2);
	}
#if (defined DEBUG_ENABLE) && (DEBUG_ENABLE == STD_TRUE)
	Sys_Debug(STD_ON);
#endif
}
/****************************************************************************************************
**Function:
	void ProtoAnaly_DeleteKeyInfo(uint8_t *pInData ,uint8_t *pOutData)
**Author: rory
**Description:
**Input: 
**Output: 
****************************************************************************************************/
void ProtoAnaly_DeleteKeyInfo(uint8_t *pInData ,uint8_t *pOutData)
{
	pOutData[0] = 2;			/* 长度*/
	pOutData += 7;
#if (defined DEBUG_ENABLE) && (DEBUG_ENABLE == STD_TRUE)
	Sys_Debug(STD_OFF);
#endif
	pOutData[1] = LockKey_Remove_Key(&pInData[6], pInData[5]);
#if (defined DEBUG_ENABLE) && (DEBUG_ENABLE == STD_TRUE)
	Sys_Debug(STD_ON);
#endif
	if(pOutData[1] == E_OK)
	{
		Access_BeepStart(BEEP_NORMAL,1);
	}
	else
	{
		Access_BeepStart(BEEP_FAST,2);
	}
}
/****************************************************************************************************
**Function:
	void ProtoAnaly_SetTouchSenvedin(uint8_t *pInData ,uint8_t *pOutData)
**Author: rory
**Description:
**Input: 
**Output: 
****************************************************************************************************/
void ProtoAnaly_SetTouchSenvedin(uint8_t *pInData ,uint8_t *pOutData)
{
	pOutData[0] = 2; /* 长度*/
	pOutData += 7;
	pOutData[1] = E_OK;
	Sys_PataCfg.touchSensitive = pInData[5];
}
/****************************************************************************************************
**Function:
	void ProtoAnaly_ConfigHostIn(uint8_t *pInData ,uint8_t *pOutData)
**Author: rory
**Description:
**Input: 
**Output: 
****************************************************************************************************/
void ProtoAnaly_RecordAckIn(uint8_t *pInData ,uint8_t *pOutData)
{
	pOutData[0] = 0; 			/* 长度*/
//	Access_OpenRedIndex+= Access_OpenRecordSendCount;
/*	if(Access_OpenRedIndex >= ACCESS_RECORD_MAX)
	{
		Access_OpenRedIndex = 0;
	}
	Access_Store_Recordx64ms = 5000/64;*/
}
/****************************************************************************************************
**Function:
	void ProtoAnaly_ConfigHostIn(uint8_t *pInData ,uint8_t *pOutData)
**Author: rory
**Description:
**Input: 
**Output: 
****************************************************************************************************/
void ProtoAnaly_LoadRecordIn(uint8_t *pInData ,uint8_t *pOutData)
{
//	uint8 total;
	uint16 index;
	
	pOutData[0] = 7+ ACCESS_RECORD_ONCE_MAX; 			/* 长度*/
	pOutData += 7;
	pOutData[1] = E_OK;
	index = BUILD_UINT16(pInData[6], pInData[5]);
	
	//Access_BeepStart(BEEP_NORMAL,1);
	
	if((AccRcord.recordnum == 0) || (AccRcord.Srecordindex >= AccRcord.recordnum))
	{/* 没有开门记录*/
		/*pOutData[2] = (AccRcord.recordnum>>8);
		pOutData[3] = (AccRcord.recordnum&0xff);
		pOutData[4] = pOutData[2];
		pOutData[5] = pOutData[3];*/
		pOutData[2] = 0;
		pOutData[3] = 0;
		pOutData[4] = pOutData[2];
		pOutData[5] = pOutData[3];
		memset(&pOutData[6], 0xff, 14);
		Access_Record_Null = 1;
		return;
	}
	if(AccRcord.Srecordindex >= ACCESS_RECORD_MAX)
	{
		AccRcord.Srecordindex = 0;
	}
	
	if(index != 0)
	{
		if(index == (AccRcord.Srecordindex+1))
		{
			AccRcord.RecordList[AccRcord.Srecordindex].Id = 0xffffffff;
			AccRcord.Srecordindex++;
			if(AccRcord.Srecordindex >= ACCESS_RECORD_MAX)
			{
				AccRcord.Srecordindex = 0;
			}
			RecordList_StorePara(0);
		}
	}
	while(1)
	{
		if(AccRcord.RecordList[AccRcord.Srecordindex].Id != 0xffffffff)
		{
			RecordList_StorePara(0);
			break;
		}
		if((AccRcord.Srecordindex<AccRcord.Wrecordindex) || (AccRcord.Srecordindex>AccRcord.Wrecordindex))
		{
			if(++AccRcord.Srecordindex>=ACCESS_RECORD_MAX)
			{
				AccRcord.Srecordindex = 0;
			}
			RecordList_StorePara(0);
		}
		else
		{/* 没有开门记录*/
			pOutData[2] = 0;
			pOutData[3] = 0;
			pOutData[4] = pOutData[2];
			pOutData[5] = pOutData[3];
			memset(&pOutData[6], 0xff, 14);
			Access_Record_Null = 1;
			return;
		}
	}
	index = AccRcord.Srecordindex;
	Access_GetRecord(AccRcord.Srecordindex,&pOutData[6]);
	pOutData[2] = (index>>8);
	pOutData[3] = (index&0xff);
	pOutData[4] = (AccRcord.recordnum>>8);
	pOutData[5] = (AccRcord.recordnum&0xff);
}
/****************************************************************************************************
**Function:
	void ProtoAnaly_RemoveRecordIn(uint8_t *pInData ,uint8_t *pOutData)
**Author: rory
**Description:
**Input: 
**Output: 
****************************************************************************************************/
void ProtoAnaly_RemoveRecordIn(uint8_t *pInData ,uint8_t *pOutData)
{
	pOutData[0] = 2; 			/* 长度*/
	pOutData += 7;
	pOutData[1] = E_OK;
	//Access_EraseRecordData();
	//Access_BeepStart(BEEP_NORMAL,1);
}
/****************************************************************************************************
**Function:
	void ProtoAnaly_GetRfParaIn(uint8_t *pInData ,uint8_t *pOutData)
**Author: rory
**Description:
**Input: 
**Output: 
****************************************************************************************************/
void ProtoAnaly_ReadVerSionIn(uint8_t *pInData ,uint8_t *pOutData)
{
	pOutData[0] = 2+5+4;	/* 长度*/
	pOutData += 7;
	pOutData[1] = E_OK;
	pOutData[2] = 0;
	pOutData[3] = VER_MAJOR;
	pOutData[4] = VER_MAJOR;
	pOutData[5] = VER_MINOR;
	pOutData[6] = DATE_YEAR;
	pOutData[7] = DATE_MONTH;
	pOutData[8] = DATE_DAY;
	pOutData[9] = CUSTOM_CODE;
}		

/****************************************************************************************************
**Function:
	void ProtoAnaly_SetPropertyBuild(uint8_t *pInData ,uint8_t *pOutData)
**Author: rory
**Description:
**Input: 
**Output: 
****************************************************************************************************/
void ProtoAnaly_FactoryTest(uint8_t *pInData ,uint8_t *pOutData)
{
		pOutData[0] = 2; /* 长度*/
		pOutData += 7;
		
		pOutData[1] = E_OK;

		switch(pInData[5])
		{
			case COMD_FATORY_ENTERN_SLEEP:
			{
				/*BLE_UPDATA_DISCONNECT_TIMER(2000/64);
				Access_CardLearnTimer64 = 0;
				Access_BeepStart(BEEP_FAST,1);*/
				Factory_State = COMD_FATORY_ENTERN_SLEEP;
				Factory_Sub = CASE_IN;
				Access_BeepStart(BEEP_FAST,1);
				//BLE_UPDATA_DISCONNECT_TIMER(2000);
				break;
			}
			case COMD_FATORY_PARA_INIT:
			{
				Sys_ParainitFirst();
				Sys_StorePara();
				Sys_McuRestFlag=STD_TRUE;
				break;
			}
			case COMD_FATORY_MOTOR_TEST:
			{
				Factory_State = COMD_FATORY_MOTOR_TEST;
				Factory_Sub = CASE_IN;
				Access_BeepStart(BEEP_FAST,1);
				break;
			}
			default: 
			{
				pOutData[1] = E_KEY_NO_SUPPORT;
				break;
			}
		}
}


const ProtoAnaly_ComdUpType ProtoAnalyAppTable[]=
{
//	COMD_SET_TIME,ProtoAnaly_SetTimeIn,
//	COMD_GET_TIME,ProtoAnaly_GetTimeIn,
	//COMD_CONFIG_HOST,ProtoAnaly_ConfigHostIn,

//	COMD_SET_RF_PARA,ProtoAnaly_SetRfParaIn,
//	COMD_GET_RF_PARA,ProtoAnaly_GetRfParaIn,

//	CMD_O2O_READ_ID, ProtoAnaly_ReadDeviceInfoIn,
//	COMD_MODIFY_CONFIG,  ProtoAnaly_ConfigDeviceIn,
//	COMD_READ_CONFIG,  ProtoAnaly_ReadDeviceParaIn,
	COMD_MODIFY_PWSD, ProtoAnaly_ModifyPaswdIn,  

	COMD_LOCK_UP,ProtoAnaly_PowerOnIn,
	COMD_LOCK_DOWN,ProtoAnaly_PowerOffIn,
	
	COMD_SET_TIME_APP,ProtoAnaly_SetTimeIn,
	COMD_GET_TIME_APP,ProtoAnaly_GetTimeIn,
	COMD_UPLOAD_RECORD,ProtoAnaly_RecordAckIn,
	//CMD_O2O_OPEN_DEVICE_EXT, ProtoAnaly_AppOpenIdIn,
#if(SUPPORT_RECORD_LOC_STORE == STD_TRUE)
	COMD_LOAD_RECORD,ProtoAnaly_LoadRecordIn,
#endif
	CMD_O2O_READ_OTHER_KEY, ProtoAnaly_ReadKeyInfo,  // 增加密码/卡片23
	CMD_O2O_ADD_OTHER_KEY, ProtoAnaly_AddKeyInfo,
	CMD_O2O_DELETE_OTHER_KEY, ProtoAnaly_DeleteKeyInfo,
	COMD_SET_LIST,ProtoAnaly_SetListIn,
	//COMD_GET_LIST,ProtoAnaly_GetListIn,
	COMD_REMOVE_ID,ProtoAnaly_RomveIdin,
	COMD_O2O_SET_TOUCH_SEN,ProtoAnaly_SetTouchSenvedin,
	//COMD_SET_DECRP_WORD,ProtoAnaly_SetDecrpWord,
	//COMD_SET_PROPERTY_BUILD_ID,ProtoAnaly_SetPropertyBuild,
	//COMD_GET_PROPERTY_BUILD_ID,ProtoAnaly_GetPropertyBuild,
	COMD_READ_CONFIG_NEW, ProtoAnaly_ReadDeviceInfoExtIn,
	COMD_SET_CONFIG_NEW, ProtoAnaly_ConfigDeviceExtIn,
	CMD_O2O_READ_VERSION,ProtoAnaly_ReadVerSionIn,
	COMD_FACTORY_TEST,ProtoAnaly_FactoryTest,
};
#define PROTOANALY_APPCOMMAND_NUM (sizeof(ProtoAnalyAppTable) / sizeof(ProtoAnalyAppTable[0]))

//mac层处理列表
const ProtoAnaly_ComdUpType ProtoAnalyMacTable[]=
{
//	COMD_SET_REST,ProtoAnaly_SetRstIn,
	COMD_SET_PARA_DEF,ProtoAnaly_SetParaDefIn,
//	COMD_RESET_DEVICE, ProtoAnaly_SetRstIn,
};
#define PROTOANALY_MACCOMMAND_NUM (sizeof(ProtoAnalyMacTable) / sizeof(ProtoAnalyMacTable[0]))
/****************************************************************************************************
**Function:
Std_ReturnType ProtoAnaly_CmdAppProc(Proto_NetcommType netype,uint8 *pindat,uint8 *poutdat)
**Author: lxw
**Description:   0 透传  1    命令被处理 3 命令无效 交由下一级处理
**Input: 
**Output: 
****************************************************************************************************/
uint8	ProtoAnaly_CmdMacProc(Proto_NetcommType netype,uint8 *pindat,uint8 *poutdat)
{
	 uint8 i;
	 for(i= 0;i< PROTOANALY_MACCOMMAND_NUM;i++)
	 {
		 if(ProtoAnalyMacTable[i].ComdId == pindat[1])
		 {//是否支持mac 处理
			 pindat += 6;	   
			 if(&(ProtoAnalyMacTable[i].comdinmanage)== NULL)
			 {
				 break;
			 }			 
			 ProtoAnalyMacTable[i].comdinmanage(&pindat[1],poutdat);
			 poutdat[1] = ProtoAnalyMacTable[i].ComdId;
			 return 1;
		 }
	 }
	 return 2;
}
 /****************************************************************************************************
 **Function:
	  Std_ReturnType ProtoAnaly_CmdAppProc(Proto_NetcommType netype,uint8 *pindat,uint8 *poutdat)
 **Author: lxw
 **Description:  
 **Input: 
 **Output: 
 ****************************************************************************************************/
Std_ReturnType ProtoAnaly_CmdAppProc(Proto_NetcommType netype,uint8 *pindat,uint8 *poutdat)
{
	uint8 i;
	for(i= 0;i< PROTOANALY_APPCOMMAND_NUM;i++)
	{
		if(ProtoAnalyAppTable[i].ComdId == pindat[1])
		{
			pindat += 6;	 
			if(&(ProtoAnalyAppTable[i].comdinmanage)== NULL)
			{
				break;
			}
			ProtoAnalyAppTable[i].comdinmanage(&pindat[1],poutdat);
			poutdat[1] = ProtoAnalyAppTable[i].ComdId;
			return E_OK;
		}
	}
	return E_NOT_OK;
}
/****************************************************************************************************
**Function:
   Std_ReturnType ProtoAnaly_ProcEnd(Proto_NetcommType netype,uint8 cmd)
**Author: lxw
**Description:  
**Input: 
**Output: 
****************************************************************************************************/
void ProtoAnaly_ProcEnd(Proto_NetcommType netype,uint8 cmd)
{
	if((cmd == COMD_SET_REST)||(cmd == COMD_SET_PARA_DEF)||(cmd == COMD_SET_DECRP_WORD)
	||(cmd == COMD_CONFIG_HOST)||(cmd == COMD_SET_DECRP_WORD) || (cmd == COMD_MODIFY_CONFIG)
	||(cmd == COMD_O2O_SET_TOUCH_SEN)  ||(cmd == COMD_SET_LIST)
	||(cmd == COMD_REMOVE_ID))
	{
		Sys_StoreFlag=1;
	}
	if((cmd == COMD_SET_REST)||(cmd == COMD_SET_PARA_DEF) ||(cmd == COMD_SET_DECRP_WORD)
	||(cmd == COMD_MODIFY_CONFIG) ||(cmd == COMD_O2O_SET_TOUCH_SEN) ||(cmd == COMD_SET_RF_PARA))
	{
		Sys_McuRestFlag=1;
	}
	Proto_NetcommBack = netype;
}
/****************************************************************************************************
**Function:
   void ProtoAnaly_AddRecordCmd(uint8* pKeyId,uint32 time,uint8 type, uint8 action)
**Author: lxw
**Description:  
**Input: 
**Output: 
****************************************************************************************************/
void ProtoAnaly_AddRecordCmd(uint8* pKeyId,uint32 time,uint8 type, uint8 action)
{
	if(ProtoAnalyRecordIndex>= PROTOANALY_RECORD_CACHEMAX)
	{
		return ;
	}
	ProtoAnalyRecordBuf[ProtoAnalyRecordIndex][0]= type;
	if((type == KEY_TYPE_IDENTITY_CARD) || (type == ACCESS_LEARN_IDENTITY_CARD_TPYE))
	{
		memcpy(&ProtoAnalyRecordBuf[ProtoAnalyRecordIndex][1],pKeyId,8);
		ProtoAnalyRecordBuf[ProtoAnalyRecordIndex][9] = action;
		Sys_GenFun32To8(time,&ProtoAnalyRecordBuf[ProtoAnalyRecordIndex][10]);
		ProtoAnalyRecordBuf[ProtoAnalyRecordIndex][14] = Access_BatteryData;
	}
	else 
	{
		memcpy(&ProtoAnalyRecordBuf[ProtoAnalyRecordIndex][1],pKeyId,4);
		ProtoAnalyRecordBuf[ProtoAnalyRecordIndex][5] = action;
		Sys_GenFun32To8(time,&ProtoAnalyRecordBuf[ProtoAnalyRecordIndex][6]);
		ProtoAnalyRecordBuf[ProtoAnalyRecordIndex][10] = Access_BatteryData;
	}
	ProtoAnalyRecordIndex++;
}
/****************************************************************************************************
**Function:
   void ProtoAnaly_LoraSlaveCallBackProc(uint32 id)
**Author: lxw
**Description:  
**Input: 
**Output: 
****************************************************************************************************/
uint8 ProtoAnaly_RecordGetCmd(uint8 *pdat)
{
	uint8 i;
	uint8 dat;
	uint8 lenth;
	uint8 recbuf[20];
	if(ProtoAnalyRecordIndex== 0)
	{
		return 0;
	}
	else 
	{
		++ProtoAnalyRecordSendErr;
		if(ProtoAnalyRecordSendErr >3)
		{
			ProtoAnaly_RecordRemoveCmd();
			ProtoAnalyRecordSendErr = 0;
			if(ProtoAnalyRecordIndex == 0)
			{
				return 0;
			}
		}
		Sys_GenFun32To8(Sys_PataCfg.Mac,&pdat[0]);
		pdat[4]= (COMD_LORA_RECORE);
		if((ProtoAnalyRecordBuf[0][0] == KEY_TYPE_IDENTITY_CARD) || (ProtoAnalyRecordBuf[0][0] == ACCESS_LEARN_IDENTITY_CARD_TPYE))
		{
			memcpy(&pdat[5],&ProtoAnalyRecordBuf[0][0],10+4);
			lenth = (10+3+4+4);
			pdat[19]= Access_BatteryData;
			pdat[20]= Access_LockStatus; /* 门状态  1:开 0:关*/
			pdat[21]=Sys_GenFunLChecksum(lenth,pdat);
			lenth++;
			ProtoAnaly_LoraEncrypt(lenth,pdat);
			return lenth;
		}
		else 
		{
			memcpy(&pdat[5],&ProtoAnalyRecordBuf[0][0],6+4);
			lenth = (6+3+4+4);
			pdat[15]= Access_BatteryData;
			pdat[16]= Access_LockStatus; /* 门状态  1:开 0:关*/
			pdat[17]=Sys_GenFunLChecksum(lenth,pdat);
			lenth++;
			ProtoAnaly_LoraEncrypt(lenth,pdat);
			return lenth;
		}
	}
}
/****************************************************************************************************
**Function:
   void ProtoAnaly_LoraSlaveRemoveCmd(void)
**Author: lxw
**Description:  
**Input: 
**Output: 
****************************************************************************************************/
void ProtoAnaly_RecordRemoveCmd(void)
{
	uint8 i;
	if(ProtoAnalyRecordIndex == 0)
	{
		return ;
	}
	for(i= 0;i< ProtoAnalyRecordIndex;i++)
	{
		memcpy(ProtoAnalyRecordBuf[i],ProtoAnalyRecordBuf[i+1],12);
	}
	ProtoAnalyRecordIndex--;
	ProtoAnalyRecordSendErr=0;
}
/****************************************************************************************************
**Function:
	void ProtoAnaly_ModifyCustomInfo(uint8_t *pInData)
**Author: rory
**Description:
**Input: 
**Output: 
****************************************************************************************************/
void ProtoAnaly_ModifyCustomInfo(uint8_t *pInData)
{
	uint8 checksum;
	uint8 i,length;
	uint8 temp_buf[8];

	memcpy(temp_buf, &pInData[PROTO_COMM_COMD_CMDADD], 8);
	Encpt_DecrpytExt(temp_buf);
	memcpy(&pInData[PROTO_COMM_COMD_CMDADD], temp_buf, 8);
	checksum = 0;
	length = 28;
	for(i=0; length > 0; length--)
	{
		checksum ^= pInData[PROTO_COMM_COMD_LENTHADD+i];
		i++;
	}
	if(checksum != pInData[30])
	{
		Access_BeepStart(BEEP_FAST,1);
		return;
	}
	if(pInData[PROTO_COMM_COMD_CMDADD] != COMD_MODIFY_CUSTOM_INFO)
	{
		Access_BeepStart(BEEP_FAST,1);
		return;
	}
	if(Sys_PataCfg.Mac != Sys_GenFun8To32(&pInData[6]))
	{
		Access_BeepStart(BEEP_FAST,1);
		return;
	}
	Sys_PataCfg.SysId = BUILD_UINT16(pInData[5], pInData[4]);
	memcpy(Sys_PataCfg.Aeskey, &pInData[14], 16);
	Sys_StorePara();
	Access_BeepStart(BEEP_OPEN,1);
	Sys_McuRestFlag =STD_TRUE;//复位
}
/****************************************************************************************************
**Function:
	void ProtoAnaly_HextoAsc(uint8 *pInData,uint8 *pOutData,uint8 lenth)
**Author: rory
**Description:
**Input: 
**Output: 
****************************************************************************************************/
void ProtoAnaly_HextoAsc(uint8 *pInData,uint8 *pOutData,uint8 lenth)
{
   uint8 i;
   uint8 halfbyte;
   for(i= 0;i< lenth;i++)
   {
	   halfbyte= *pInData>>4;
	   if((halfbyte>=0 )&&(halfbyte<=9 ))
	   {
		   *pOutData = '0'+halfbyte;
	   }
	   else 
	   {
		   *pOutData = 0x37+halfbyte;
	   }
	   pOutData++;
	   halfbyte= *pInData&0x0f;
	   if((halfbyte>=0 )&&(halfbyte<=9 ))
	   {
		   *pOutData = '0'+halfbyte;
	   }
	   else 
	   {
		   *pOutData = 0x37+halfbyte;
	   }
		pOutData++;
		pInData++;
   }
}
/****************************************************************************************************
**Function:
	uint32 BKDRHash(uint8 *pbuf,uint8 lenth)
**Author: lxw
**Description:
**Input: 
**Output: 
****************************************************************************************************/
uint32 BKDRHash(uint8 *pbuf,uint8 lenth)
{
    char cbuf[20];
    uint32 seed = 131313; // 31 131 1313 13131 131313 etc..
    uint32 hash = 0;
    uint8 i= 0;

    memset(cbuf,0,20);
    ProtoAnaly_HextoAsc(pbuf,cbuf,lenth);
    while (cbuf[i])
    {
        hash = hash * seed + (cbuf[i++]);
    }
    return hash;
}

static uint8 reclorabuf[128];
static uint8 loraindex = 0;
/****************************************************************************************************
**Function:
	void ProtoAnaly_PublicProc(uint8 *pInData,uint8 *pOutData,uint8 lenth)
**Author: rory
**Description:	
**Input: 
**Output: 
****************************************************************************************************/
Std_ReturnType ProtoAnaly_PublicProc(Proto_NetcommType netype,uint8 *pindat,uint16 lenth,uint8 *poutdat)
{
#if (defined LORA_ENABLE) && (LORA_ENABLE == STD_TRUE) 
#if 0
	uint8 ret = E_OK;
	uint8 cmd;
	uint32 crcmac;
	uint8 index;
	uint8 sendindex;
	uint8 delayRecflag;
	uint8 reack = STD_FALSE;
	if(netype == LORA)
	{
		ProtoAnaly_LoraDecrpyt(lenth,pindat);
		if(pindat[lenth-1] != Sys_GenFunLChecksum(lenth-1,pindat))
		{
			ProtoAnaly_LoraSlaveTickSnyc(2,0,0);
			return E_OK;
		}
		index = pindat[lenth-3];
		sendindex = pindat[lenth-2];		
		cmd = pindat[4+4];
		crcmac = Sys_GenFun8To32(&pindat[4]);
		if(crcmac == ProtoAnalyHashKey)
		{
			reack= STD_TRUE;
		}
		else if((lenth == 28)&&(crcmac == 0xffffffff))
		{
			//28 字节特有的注册列表指令优先级高于其他公共指令
			// 注册列表是用的公共消息
			if((Sys_GenFun8To32(&pindat[9]) == ProtoAnalyHashKey)||((ProtoAnaly_HashKeyInit(0x12345678) == Sys_GenFun8To32(&pindat[9]) ) &&(ProtoAnaly_HashKeyInit(BUILD_UINT32(pindat[16], pindat[15], pindat[14], pindat[13])) == ProtoAnalyHashKey)))
			{
				if(Sys_PataCfg.Pswd == 0x12345678)
				{
					Sys_PataCfg.Pswd = BUILD_UINT32(pindat[16], pindat[15], pindat[14], pindat[13]);
					ProtoAnalyHashKey = ProtoAnaly_HashKeyInit(Sys_PataCfg.Pswd ) ;
					Sys_StoreFlag = STD_TRUE;
					if((Sys_PataCfg.SyncIndex != ((sendindex<<1) +1))||(Sys_PataCfg.HeadMac  != Sys_GenFun8To32(&pindat[0])))
					{
						Sys_PataCfg.SyncIndex = (sendindex<<1) +1;
						Sys_PataCfg.HeadMac = Sys_GenFun8To32(&pindat[0]);
					}
				}
				else 
				{
					if((Sys_PataCfg.SyncIndex != ((sendindex<<1) +1))||(Sys_PataCfg.HeadMac  != Sys_GenFun8To32(&pindat[0])))
					{
						Sys_PataCfg.SyncIndex = (sendindex<<1) +1;
						Sys_PataCfg.HeadMac = Sys_GenFun8To32(&pindat[0]);
						Sys_StoreFlag = STD_TRUE;
					}
				}
				Access_BeepStart(BEEP_NORMAL,1);
			}
			else if((Sys_GenFun8To32(&pindat[17]) == ProtoAnalyHashKey)||((ProtoAnaly_HashKeyInit(0x12345678) == Sys_GenFun8To32(&pindat[17]) ) &&(ProtoAnaly_HashKeyInit(BUILD_UINT32(pindat[24], pindat[23], pindat[22], pindat[21])) == ProtoAnalyHashKey)))
			{
				if(Sys_PataCfg.Pswd == 0x12345678)
				{
					Sys_PataCfg.Pswd = BUILD_UINT32(pindat[24], pindat[23], pindat[22], pindat[21]);
					ProtoAnalyHashKey = ProtoAnaly_HashKeyInit(Sys_PataCfg.Pswd ) ;
					Sys_StoreFlag = STD_TRUE;
					if((Sys_PataCfg.SyncIndex != ((sendindex<<1) +2))||(Sys_PataCfg.HeadMac  != Sys_GenFun8To32(&pindat[0])))
					{
						Sys_PataCfg.SyncIndex = (sendindex<<1) +2;
						Sys_PataCfg.HeadMac = Sys_GenFun8To32(&pindat[0]);
					}
				}
				else 
				{
					if((Sys_PataCfg.SyncIndex != ((sendindex<<1) +2))||(Sys_PataCfg.HeadMac  != Sys_GenFun8To32(&pindat[0])))
					{
						Sys_PataCfg.SyncIndex = (sendindex<<1) +2;
						Sys_PataCfg.HeadMac = Sys_GenFun8To32(&pindat[0]);
						Sys_StoreFlag = STD_TRUE;
					}
				}
				Access_BeepStart(BEEP_NORMAL,1);
			}
			else 
			{
				//下发列表 启动持续接收
				ProtoAnaly_LoraSlaveTickSnyc(0,index,0);
				return E_OK;
			}
			if(Sys_PataCfg.rfPara.rfreq !=pindat[25])
			{
				if(pindat[25] < LORA_FREQ_CHANNEL_MAX )
				{
					Sys_PataCfg.rfPara.rfreq =pindat[25];
					Sys_StoreFlag = STD_TRUE;
				}
			}	
			ProtoAnaly_LoraSlaveTickSnyc(2,0,0);
			return E_OK;
		}
		else if((Sys_PataCfg.HeadMac ==  Sys_GenFun8To32(&pindat[0]))&&(crcmac == 0xffffffff))
		{
			// 公共消息
			delayRecflag =STD_TRUE;
		}
		else if(Sys_PataCfg.HeadMac ==  Sys_GenFun8To32(&pindat[0]))
		{
			//私有消息 作为Beacon包
			cmd = COMD_LORA_SNYC;
		}
		else 
		{
			ProtoAnaly_LoraSlaveTickSnyc(2,0,0);
			return E_OK;
		}
		pindat+= 4;
		switch(cmd)
		{			
			case COMD_LORA_SNYC:
			{
				ProtoAnaly_LoraSlaveTickSnyc(1,index,Sys_PataCfg.SyncIndex);
			}return E_OK ;
			case COMD_LORA_OPEN:
			{
				ret = ProtoAnaly_CheckTime(&pindat[5]);
				if(ret == E_OK)
				{
					if(pindat[9] == 0x00)
					{     // 常闭
						power_out_sleep();
						Access_BeepStart(BEEP_OPEN,1);
						//Access_MotorClose();
						Sys_PataCfg.State = WORK_NORMAL;
			                     Access_LockStatus |= DOOR_MOTOR_ST;
						Access_Globle.ErrorTimes = 0;
						Access_LockDeviceTimer64ms = 0;
						Access_LockTimer64 = 0;
						Sys_StoreFlag = STD_TRUE;
						ProtoAnaly_LoraSlaveTickSnyc(2,0,0);
						return E_OK;
					}
					else if(pindat[9] == 0xff)
					{	// 常开
					       power_out_sleep();
						Access_BeepStart(BEEP_OPEN,1);
						//Access_MotorOpen();
						Sys_PataCfg.State = WORK_NORMALLY_OPEN;
			                     Access_LockStatus &= ~DOOR_MOTOR_ST;
						Access_Globle.ErrorTimes = 0;
						Access_LockDeviceTimer64ms = 0;
						Access_LockTimer64 = 0;
						Sys_StoreFlag = STD_TRUE;
						ProtoAnaly_LoraSlaveTickSnyc(2,0,0);
						return E_OK;
					}
					else 
					{
					#if (defined(CLASS_ROOM_ENABLE)&&(CLASS_ROOM_ENABLE == STD_TRUE))
						power_out_sleep();
						Access_Unlock();
					#else
						// 正常开锁				
						if(Sys_PataCfg.State == WORK_NORMALLY_OPEN)
						{
							Access_BeepStart(BEEP_LOW,BEEP_ERROR_EMERGENCY);
							break;
						}
						power_out_sleep();
						Access_Unlock();
					#endif
						Access_UpdateStatus();
					}
				}
				else 
				{
					Access_BeepStart(BEEP_LOW,1);
				}
				SysBasetimer_Start(10);
				goto ACK_SEND;
			}break;	
			case COMD_LORA_SET_TIME:
			{
				uint32 timedat;
				timedat= BUILD_UINT32(pindat[8], pindat[7],pindat[6], pindat[5]);
				ProtoAnaly_LoraSetTime(timedat);
				sendindex= Sys_PataCfg.SyncIndex-1;
				reack= STD_TRUE;
				goto ACK_SEND;
			}break;
			case COMD_LORA_RECORE:
			{
				ProtoAnaly_RecordRemoveCmd();
			}break;
			case COMD_LORA_SETLIST:
			{
				if((pindat[5] == KEY_TYPE_ONE_TIME) || (pindat[5] == KEY_TYPE_FOREVER)
				|| (pindat[5] == KEY_TYPE_TIME_SLOT_7) || (pindat[5] == KEY_TYPE_CLEAR)
				|| (pindat[5] == KEY_TYPE_TIME_SLOT_8))
				{
					uint32 keyId = BUILD_UINT32(pindat[10], pindat[9], pindat[8], pindat[7]);
					BlackList_RemoveKey(pindat[5], keyId);
					return E_OK;
				}
				ret = ProtoAnaly_AddId(pindat[5],&pindat[7],pindat[6]);
				Access_BeepStart(BEEP_FAST,1);
				goto ACK_SEND;
			}break;			
			case COMD_LORA_REMOVE:
			{
				ret = ProtoAnaly_RemoveId(pindat[5],&pindat[6]);
				Access_BeepStart(BEEP_FAST,1);
				goto ACK_SEND;
			}break;			
			case COMD_LORA_REST:
			{
				Sys_McuRestFlag= 1;
				goto ACK_SEND;
			}break;			
			case COMD_LORA_CFGRF:
			{
				if(Sys_PataCfg.rfPara.rfreq !=pindat[23])
				{
					if(pindat[5] < LORA_FREQ_CHANNEL_MAX )
					{
						Sys_PataCfg.rfPara.rfreq =pindat[23];
						Sys_StoreFlag = STD_TRUE;
					}
				}
				Access_BeepStart(BEEP_NORMAL,1);
				sendindex= Sys_PataCfg.SyncIndex-1;
				reack= STD_TRUE;
				goto ACK_SEND;
			}break;	
			case COMD_LORA_ADDKEY:
			{
				if(LockKeyStatistics.ServKeyCount > SERVE_KEY_MAX)
				{
					ret = E_NOT_OK;
				}
				if((pindat[5] == KEY_TYPE_CARD) || (pindat[5] == KEY_TYPE_IDENTITY_CARD))
				{
					ret = LockKey_Start_LearnCardKeyExt(&pindat[6]);
				}
				else
				{
					ret = E_INPAR_ERR;
				}
				Access_BeepStart(BEEP_FAST,1);
				power_out_sleep();
				goto ACK_SEND;
			}break;	
			default:return E_OK;
		}
		ACK_SEND:
			if(reack == STD_TRUE)
			{
				Sys_GenFun32To8(Sys_PataCfg.Mac,&ProtoAnaly_LoraBrgBuf[0]);
				ProtoAnaly_LoraBrgBuf[4]= cmd;
				ProtoAnaly_LoraBrgBuf[5]= ret;
				ProtoAnaly_LoraBrgBuf[6]= Access_BatteryData;
				ProtoAnaly_LoraBrgBuf[7]= Access_LockStatus;
				ProtoAnaly_LoraBrgSize= 9;
				ProtoAnaly_LoraBrgBuf[ProtoAnaly_LoraBrgSize-1]=Sys_GenFunLChecksum(ProtoAnaly_LoraBrgSize-1,ProtoAnaly_LoraBrgBuf);
				ProtoAnaly_LoraEncrypt(ProtoAnaly_LoraBrgSize,&ProtoAnaly_LoraBrgBuf[0]);
				ProtoAnaly_LoraSendTims= 1;
				LoraWakeup_timer_Start(RADIO_SET_TX,(PROTOANALY_REC_WINDOWTIMES*PROTOANALY_CYCLETIMES_MAX-((index+1)*PROTOANALY_REC_WINDOWTIMES*(1<<(Sys_PataCfg.rfPara.Speed)))) + sendindex*PROTOANALY_SEND_WINDOWTIMES*(1<<(Sys_PataCfg.rfPara.Speed)) +50); 			
			}
			else 
			{
				ProtoAnaly_LoraSlaveTickSnyc(2,0,0);
			}
		return E_OK;
	}
#endif
#endif
	return E_NOT_OK;
}
/****************************************************************************************************
**Function:
   void ProtoAnaly_LoraSlaveCallBackProc(void)
**Author: lxw
**Description:  
**Input: 
**Output: 
****************************************************************************************************/
void ProtoAnaly_LoraSlaveCallBackProc(void)
{
	uint8 datbuf[30]={0};
	datbuf[0] = 4;
	datbuf[1] = COMD_GET_HEARTBEAT;
	datbuf[8] = 0;
	datbuf[9] = TYPE_LOCK_ACCESS_REMOTE;
	datbuf[10] = 0;
	Proto_CommSend(LORA,datbuf);
}
#if (defined LORA_ENABLE) && (LORA_ENABLE == STD_TRUE)
uint8 LoraRecSnytimes;
/****************************************************************************************************
**Function:
   void ProtoAnaly_LoraSlaveTickSnyc(uint8 maxunm,uint8 index)
**Author: lxw
**Description:  
**Input: 
**Output: 
****************************************************************************************************/
void ProtoAnaly_LoraSlaveTickSnyc(uint8 wktype,uint8 index,uint8 dereindex)
{
	int16 delaytime;
	if(wktype == 0)
	{
		LoraRcing_timer_Delay(10);
	}
	else if(wktype == 1)
	{
		if(((Sys_PataCfg.SyncIndex-1)&0x07) == ((index&0x07)))
		{
			delaytime= (((((PROTOANALY_CYCLETIMES_MAX >>Sys_PataCfg.rfPara.Speed)-(index+1))>>3)<<3)-1)*(PROTOANALY_REC_WINDOWTIMES *(1<<Sys_PataCfg.rfPara.Speed))-30;
			if((delaytime >0 )&&((index + Sys_PataCfg.SyncIndex)<(PROTOANALY_CYCLETIMES_MAX >>Sys_PataCfg.rfPara.Speed)))
			{
				LoraWakeup_timer_Start(RADIO_SET_RX,(uint16)delaytime);
			}
			else 
			{
				LoraWakeup_timer_Start(RADIO_SET_CAD_DET,(PROTOANALY_REC_WINDOWTIMES*PROTOANALY_CYCLETIMES_MAX)/2-20);
			}
		}
		else if((index + Sys_PataCfg.SyncIndex)<(PROTOANALY_CYCLETIMES_MAX >>Sys_PataCfg.rfPara.Speed))
		{
			delaytime = PROTOANALY_CYCLETIMES_ONCE_MAX*(PROTOANALY_REC_WINDOWTIMES*(1<<Sys_PataCfg.rfPara.Speed) )-(((index&0x07)+1)*(PROTOANALY_REC_WINDOWTIMES*(1<<Sys_PataCfg.rfPara.Speed) )) +(((dereindex-1) &0x07)*PROTOANALY_REC_WINDOWTIMES)-25;
			if(delaytime >0 )
			{
				LoraWakeup_timer_Start(RADIO_SET_RX,(uint16)delaytime);
			}
			else 
			{
				LoraRcing_timer_Delay(10);
			}
		}
		else 
		{
			LoraWakeup_timer_Start(RADIO_SET_CAD_DET,(PROTOANALY_REC_WINDOWTIMES*PROTOANALY_CYCLETIMES_MAX)/2-20);
		}
	}
	else if(wktype == 2)
	{
		//继续接收下个周期数据
		LoraRecSnytimes = 0;
		LoraWakeup_timer_Start(RADIO_SET_CAD_DET,PROTOANALY_REC_WINDOWTIMES*PROTOANALY_CYCLETIMES_MAX-20);
	}
	else if(wktype == 3)
	{
//		RadioDrive.ioctl(RADIO_SET_SLEEP,NULL);
	}
}
#endif
/****************************************************************************************************
**Function:
**Description:
void ProtoAnaly_LoraEncrypt(uint8 lenth,uint8 *dat)
**Input: 
**Output: 
****************************************************************************************************/
void ProtoAnaly_LoraEncrypt(uint8 lenth,uint8 *dat)
{
	uint8 i;
	uint8 checksum = 0;
#if(COMMENCPT_ENABLE == STD_TRUE)	
	for(i= 0;i< (lenth>>3);i++)
	{
		Encpt_EncryptExt(&dat[(i<<3)]);
	}
#endif
}
/****************************************************************************************************
**Function:
**Description:
Std_ReturnType ProtoAnaly_LoraDecrpyt(uint8 lenth,uint8 *dat)
**Input: 
**Output: 
****************************************************************************************************/
Std_ReturnType ProtoAnaly_LoraDecrpyt(uint8 lenth,uint8 *dat)
{
	uint8 i;
	uint8 checksum = 0;
#if   (COMMENCPT_ENABLE == STD_TRUE)
	for(i= 0;i< (lenth>>3);i++)
	{
		Encpt_DecrpytExt(&dat[i<<3]);
	}
#endif
		return E_OK;
	}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

