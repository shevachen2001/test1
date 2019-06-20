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
#include "SYSTEM\sys.h"
//#include "UART\UART.h"
#include "KEY\key.h"
#include "KEY\Touch.h"
#include "SYSTEM\Sys_TimeBase.h"
#include "Protocol\factInfo.h"
#include "Access\LockKey.h"
//#include "WatchDog\WatchDog.h"
#include "ble_dahao.h"
#include "Access\Access.h"
#include "RTC\Rtc.h"
#include "protocol\Proto_Analy.h"
#include "Flash\Flash.h"
#include "Flash\Flash_Nrf.h"

extern uint8  Access_AddId_NewFlag;
extern uint8  Access_FlashProcFlag;
extern uint32 Access_UserId_Temp;
extern uint32 Access_ScanInex_Temp;
extern uint8 Access_AttTIme_Temp[8];
extern uint32 ProtoAnaly_RtcLocalTime;
	

LockKey_StatisticsType LockKeyStatistics;
LockKey_BlackListType LockKeyBlackList;
LockKey_ServeKeyListType LockKeyServeKeyList;
//LockKey_AppKeyListType LockKeyAppKeyList;
uint8 Access_Record_Null = 0;


uint8 ServeKeyFlashOffset;

/****************************************************************************************************
**Function:
	uint8 LockKey_RemoveKey(uint8 type, uint8 index)
**Author: rory
**Description: 
**Input: 
**Output: 
****************************************************************************************************/
void LockKey_RemoveKey(uint8 idtype, uint8 index)
{	
	uint8 year, month, day;
	uint32 tempTime;
	
	if(index == 0)
	{
		return;
	}
	index--;
	if(idtype == 1)
	{
	    ServeKeyList_Readflash(ServeKeyFlashOffset);
		tempTime = LockKeyServeKeyList.KeyList[index].StartTime;
		year = BUILD_YEAR(tempTime);
		month = BUILD_MONTH(tempTime);
		day = BUILD_DAY(tempTime);
		if((ProtoAnaly_Rtcinfo.Year == year)&&(ProtoAnaly_Rtcinfo.Month == month)
		&&(ProtoAnaly_Rtcinfo.Day == day))
		{ /* 加入黑名单*/
			BlackList_Add(LockKeyServeKeyList.KeyList[index].Type, LockKeyServeKeyList.KeyList[index].KeyId,
			BUILD_TIME(ProtoAnaly_Rtcinfo.Year,ProtoAnaly_Rtcinfo.Month,ProtoAnaly_Rtcinfo.Day,
			23,59,59));
			BlackList_StorePara();
		}
		memset(&LockKeyServeKeyList.KeyList[index], 0xff, sizeof(LockKey_ServeKeyType));
		if(LockKeyStatistics.ServKeyCount > 0)
		{
			LockKeyStatistics.ServKeyCount--;
		}
	    ServeKeyList_Writeflash(ServeKeyFlashOffset);
		//ServeKeyList_StorePara1();  
	}
}


/****************************************************************************************************
**Function:
	uint16 LockKey_CommPswdId(uint8 idtpye, uint8 *pid, uint32 *pkeyId,uint8 lenth)
**Author: rory
**Description: 返回id所在色索引 
**Input: 0:黑名单  1:服务下发 2:APP下发
**Output:  0 表示没找到
****************************************************************************************************/
uint16 LockKey_CommPswdId(uint8 idtpye, uint8 *pid, uint32 *pkeyId, uint8 lenth)
{	
	uint32 keytemp,rdkeytemp, keyData;
	uint8 lenthhfbyte, currentMax,  keyType;

	if(idtpye == 0)
	{
		currentMax = LockKeyStatistics.BlackListCount;
		//totalMax = BLACK_LIST_MAX;
	}
	else if(idtpye == 1)
	{
		currentMax = LockKeyStatistics.ServKeyCount;
		//totalMax = SERVE_KEY_MAX;
	}
	else
	{
		return 0;
	}
	if(currentMax == 0)
	{
		return 0;
	}
	lenthhfbyte = lenth;
	for(;;)
	{	
		LOCKKEY_COMMAGAIN:
		keytemp = 0;
		if(lenthhfbyte >= 8)
		{
			if(lenthhfbyte&0x01)
			{
				keytemp = BUILD_UINT32(pid[(lenthhfbyte>>1)-1],pid[(lenthhfbyte>>1)-2],pid[(lenthhfbyte>>1)-3],pid[(lenthhfbyte>>1)-4]);
				keytemp<<=4;
				keytemp|=(pid[(lenthhfbyte>>1)]>>4);
			}
			else 
			{
				keytemp = BUILD_UINT32(pid[(lenthhfbyte>>1)-1],pid[(lenthhfbyte>>1)-2],pid[(lenthhfbyte>>1)-3],pid[(lenthhfbyte>>1)-4]);
			}
			lenthhfbyte--;
		}
		else 
		{
			keytemp = 0;
			if(lenthhfbyte&0x01)
			{
				for(uint8 i =0;i <(lenthhfbyte>>1)+1;i++)
				{
					keytemp<<=8;
					keytemp|= pid[i];
				}
				keytemp>>=4;
			}
			else 
			{
				for(uint8 i =0;i <(lenthhfbyte>>1);i++)
				{
					keytemp<<=8;
					keytemp|= pid[i];
				}
			}
			lenthhfbyte--;
		}
#if 1		
		for(uint16 j= 0;j< currentMax;)
		{	
		  if(idtpye == 0)
	      {
	  			for(uint8 i =0;i <BLACK_LIST_MAX;i++)  
	  	        {
	  				keyType = LockKeyBlackList.BlackList[i].Type;
	  				if( (keyType == KEY_TYPE_PASSWORD) || (keyType == KEY_TYPE_TIME_SLOT_7) || (keyType == KEY_TYPE_TIME_SLOT_8)
	  					|| (keyType == KEY_TYPE_ONE_TIME) || (keyType == KEY_TYPE_FOREVER)	|| (keyType == KEY_TYPE_CLEAR))
	  				{
	  					keyData = LockKeyBlackList.BlackList[i].KeyId;
	  					if(keyData != 0xffffffff)
	  					{
	  						if(BlackList_Remove(i) == E_OK)
	  						{// 成功删除黑名单
	  							continue;
	  						}
	  					}
	  				}
	  				else
	  				{
	  					continue;
	  				}
	  
	  
	  				if(keyData == 0xffffffff)
	  				{
	  					continue;
	  				}
	  				j += 1;
	  				rdkeytemp = keyData;
	  				if(( rdkeytemp&0xf0000000)!= 0xf0000000)
	  				{
	  					if(keytemp == rdkeytemp)
	  					{
	  						*pkeyId = rdkeytemp;
	  						return (i+1);
	  					}
	  				}
	  				else if((rdkeytemp &0xff000000)!= 0xff000000)
	  				{
	  					if((keytemp&0x0fffffff) == (rdkeytemp&0x0fffffff))
	  					{
	  						*pkeyId = (rdkeytemp&0x0fffffff)|0xf0000000;
	  						return (i+1);
	  					}
	  				}
	  				else if((rdkeytemp &0xfff00000)!= 0xfff00000)
	  				{
	  					if((keytemp&0x00ffffff) == (rdkeytemp&0x00ffffff))
	  					{
	  							*pkeyId = (rdkeytemp&0x00ffffff)|0xff000000;
	  							return (i+1);
	  					}
	  				}
	  				else if((rdkeytemp &0xffff0000)!= 0xffff0000)
	  				{
	  					if((keytemp&0x000fffff) == (rdkeytemp&0x000fffff))
	  					{
	  						*pkeyId = (rdkeytemp&0x000fffff)|0xfff00000;
	  						return (i+1);
	  					}
	  				}
	  				else if((rdkeytemp &0xffff0000)== 0xffff0000)
	  				{
	  					if((keytemp&0x0000ffff) == (rdkeytemp&0x0000ffff))
	  					{
	  						*pkeyId = (rdkeytemp&0x0000ffff);
	  						return (i+1);
	  					}
	  				}
	  			}
	        }
			else if ( idtpye == 1 )
			{
			  	for(uint8 k=0;k<SERVE_KEY_PAGEMAX;k++ )
	            {
	                ServeKeyList_Readflash( k );
					for(uint8 i =0;i <SERVE_KEY_MAX;i++)  
					{
						keyType = LockKeyServeKeyList.KeyList[i].Type;
						if((keyType == KEY_TYPE_PASSWORD) || (keyType == KEY_TYPE_TIME_SLOT_7)
							|| (keyType == KEY_TYPE_TIME_SLOT_8) || (keyType == KEY_TYPE_ONE_TIME) || (keyType == KEY_TYPE_FOREVER))
						{
						    ServeKeyFlashOffset = k;
							keyData = LockKeyServeKeyList.KeyList[i].KeyId;
							if(ServeKeyList_Remove(i) == E_OK)
							{// 成功删除
								continue;
							}
						}
						else
						{
							continue;
						}

						if(keyData == 0xffffffff)
						{
							continue;
						}
						j += 1;
						rdkeytemp = keyData;
						if(( rdkeytemp&0xf0000000)!= 0xf0000000)
						{
							if(keytemp == rdkeytemp)
							{
								*pkeyId = rdkeytemp;
								return (i+1);
							}
						}
						else if((rdkeytemp &0xff000000)!= 0xff000000)
						{
							if((keytemp&0x0fffffff) == (rdkeytemp&0x0fffffff))
							{
								*pkeyId = (rdkeytemp&0x0fffffff)|0xf0000000;
								return (i+1);
							}
						}
						else if((rdkeytemp &0xfff00000)!= 0xfff00000)
						{
							if((keytemp&0x00ffffff) == (rdkeytemp&0x00ffffff))
							{
									*pkeyId = (rdkeytemp&0x00ffffff)|0xff000000;
									return (i+1);
							}
						}
						else if((rdkeytemp &0xffff0000)!= 0xffff0000)
						{
							if((keytemp&0x000fffff) == (rdkeytemp&0x000fffff))
							{
								*pkeyId = (rdkeytemp&0x000fffff)|0xfff00000;
								return (i+1);
							}
						}
						else if((rdkeytemp &0xffff0000)== 0xffff0000)
						{
							if((keytemp&0x0000ffff) == (rdkeytemp&0x0000ffff))
							{
								*pkeyId = (rdkeytemp&0x0000ffff);
								return (i+1);
							}
						}
					  }
			  	   }  
			   }
			
			if(lenthhfbyte < 4)
			{
				return 0;
			}
			else 
			{
				goto LOCKKEY_COMMAGAIN;
			}
		}
#endif		
	}
}

/****************************************************************************************************
**Function:
	uint8 LockKey_Check_PaswdKey(uint8 *pInData)
**Author: rory
**Description:
**Input: 
**Output: 
****************************************************************************************************/
uint8 LockKey_Check_PaswdKey(uint8* pTouchKey,uint8 lenth )
{
	uint8 index;
	uint8 temp_buf[8];
	uint32 keyId, startTime, endTime;
	uint8 ret, idtype, keyType,cycledat;

	index =  LockKey_CommPswdId(0, pTouchKey, &keyId, lenth);
	if(index != 0)
	{
		return E_TIME_OUT;
	}
	index =  LockKey_CommPswdId(1, pTouchKey, &keyId, lenth);
	if(index != 0)
	{
		startTime = LockKeyServeKeyList.KeyList[index-1].StartTime;
		endTime = LockKeyServeKeyList.KeyList[index-1].EndTime;
		keyType = LockKeyServeKeyList.KeyList[index-1].Type;
		cycledat =  LockKeyServeKeyList.KeyList[index-1].Cycle;
		idtype = 1;
	}
	else
	{
		return E_NOT_OK;
	}
#if (defined RTC_EN) && (RTC_EN == STD_TRUE)
	ret = Access_ComTime(startTime, endTime);
	if(ret == E_TIME_OUT)
	{
		LockKey_RemoveKey(idtype, index);
	}
	if((cycledat&0x80) ==0)
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
		if((cycledat&(1<<t)) ==0)
		{
			return E_TIME_OUT;
		}
		if(Access_ComTimeLoop(startTime, endTime) != E_OK)
		{
			return E_TIME_OUT;
		}
	}
#else 
	return E_OK;
#endif
	Sys_GenFun32To8(keyId, temp_buf);
#if (defined(SUPPORT_RECORD_LOC_STORE)&&(SUPPORT_RECORD_LOC_STORE == STD_TRUE))
	Access_WriteRecordFlash(temp_buf,ProtoAnaly_RtcLocalTime,keyType,ACCESS_OPEN_LOCK_TPYE);
#endif
#if (SUPPORT_RECORD_RTIME_SEND == STD_TRUE)
	ProtoAnaly_AddRecordCmd(temp_buf,ProtoAnaly_RtcLocalTime,keyType,ACCESS_OPEN_LOCK_TPYE);
#endif
	return ret;
}


/****************************************************************************************************
**Function:
	uint8 LockKey_Check_CardKey(uint8 idtpye,uint8 *pid)
**Author: rory
**Description:
**Input: 
**Output: 
****************************************************************************************************/
uint8 LockKey_Check_CardKey(uint8 idtpye,uint8 *pid)
{
	uint32 keyId, keyData;
	uint8 ret=E_NOT_OK;

	if(idtpye == KEY_TYPE_IDENTITY_CARD)
	{
		keyId = BUILD_UINT32(pid[7],pid[6],pid[5],pid[4]);
	}
	else
	{
	keyId = BUILD_UINT32(pid[3], pid[2], pid[1], pid[0]);
	}
	/*  检测黑名单*/
	for(uint8 i=0; i<BLACK_LIST_MAX; i++)
	{
		if((LockKeyBlackList.BlackList[i].Type == KEY_TYPE_CARD) || (LockKeyBlackList.BlackList[i].Type == KEY_TYPE_IDENTITY_CARD))
		{
			if(LockKeyBlackList.BlackList[i].KeyId != 0xffffffff)
			{
				if(BlackList_Remove(i) == E_OK)
				{// 成功删除黑名单
					continue;
				}
			}
		}
		else
		{
			continue;
		}
		if(idtpye == LockKeyBlackList.BlackList[i].Type)
		{
			if(LockKeyBlackList.BlackList[i].KeyId == keyId)
			{
				return E_TIME_OUT;
			}
		}
	}
	if((LockKeyStatistics.ServKeyCount == 0) && (LockKeyStatistics.AppKeyCount == 0))
	{
		return E_NOT_OK;
	}
	for (uint8 j=0;j<SERVE_KEY_PAGEMAX;j++ )
	{
	    ServeKeyList_Readflash( j );	
		for(uint8 i=0; i<SERVE_KEY_MAX; i++)
		{
			if(idtpye == LockKeyServeKeyList.KeyList[i].Type)
			{
				if(LockKeyServeKeyList.KeyList[i].KeyId == keyId)
				{
				#if (defined RTC_EN) && (RTC_EN == STD_TRUE)
					uint8 t = 0;

					if(ProtoAnaly_Rtcinfo.Week == 7)
					{
						t = 0;
					}
					else
					{
						t = ProtoAnaly_Rtcinfo.Week;
					}
					ret = Access_ComTime(LockKeyServeKeyList.KeyList[i].StartTime, LockKeyServeKeyList.KeyList[i].EndTime); 
					if(ret != E_OK) 
					{
						return E_NOT_OK;
					}
					else if(LockKeyServeKeyList.KeyList[i].Cycle&0x80)
					{
						return E_OK;
					}
					else if(LockKeyServeKeyList.KeyList[i].Cycle & (1<<t))
					{
						if(Access_ComTimeLoop(LockKeyServeKeyList.KeyList[i].StartTime, LockKeyServeKeyList.KeyList[i].EndTime) == E_OK)
						{
							return E_OK;
						}
						else
						{
							return E_NOT_OK;
						}
					}
				#else
					return E_OK;
				#endif
				}
			}
		}
	}
	return E_NOT_OK;
}

/****************************************************************************************************
**Function:
	uint8 LockKey_Remove_PasswordKey(BlueKey_AllType BlueKey)
**Author: rory
**Description:
**Input: type: 0(All)  1(one)
             limitTime:限制时间
**Output: 
****************************************************************************************************/
uint8 LockKey_Remove_PasswordKey(void)
{
	uint8 offset;
	uint8 year,month,day;
	uint32 tempTime;
	for (uint8 j=0;j<SERVE_KEY_PAGEMAX;j++ )
	{
	    ServeKeyList_Readflash( j );	

		for(uint8 i=0; i<SERVE_KEY_MAX; i++)
		{
			if(LockKeyServeKeyList.KeyList[i].KeyId != 0xffffffff)
			{
				if((LockKeyServeKeyList.KeyList[i].Type == KEY_TYPE_TIME_SLOT_8)
					|| (LockKeyServeKeyList.KeyList[i].Type == KEY_TYPE_FOREVER)
					|| (LockKeyServeKeyList.KeyList[i].Type == KEY_TYPE_ONE_TIME)
					|| (LockKeyServeKeyList.KeyList[i].Type == KEY_TYPE_TIME_SLOT_7))
				{
				    offset = j;
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
					}
				}
				memset(&LockKeyServeKeyList.KeyList[i], 0xff, sizeof(LockKey_ServeKeyType));
			}
		}
	}
	BlackList_StorePara();
	ServeKeyList_Writeflash( offset );	
	//ServeKeyList_StorePara1();//
	return E_OK;
}

/****************************************************************************************************
**Function:
	uint8 LockKey_Add_VisitCodeKey(uint8 *pData)
**Author: rory
**Description:
**Input: 
**Output: 
****************************************************************************************************/
uint8 LockKey_Add_VisitCodeKey(uint8 idtype,uint32 keyId,  uint32 startTime, uint32 endTime)
{
	uint8 buftemp[13];
	uint8 err = E_NOT_OK;

	Sys_GenFun32To8(keyId, &buftemp[0]);
	Sys_GenFun32To8(endTime, &buftemp[4]);
	//buftemp[8] = 0;
	Sys_GenFun32To8(startTime, &buftemp[8]);
	buftemp[12] = 0xff;
	err = ProtoAnaly_AddId(idtype, buftemp, 1);
	return err;
}

/****************************************************************************************************
**Function:
	uint8 LockKey_Add_BlueKey(BlueKey_AllType BlueKey)
**Author: rory
**Description:
**Input: type: 0(All)  1(one)
             limitTime:限制时间
**Output: 
****************************************************************************************************/
uint8 LockKey_Remove_VisitCodeKey(void)
{
	uint8 offset;
	uint8 year,month,day;
	uint32 tempTime;
	for (uint8 j=0;j<SERVE_KEY_PAGEMAX;j++ )
	{
	    ServeKeyList_Readflash( j );	
		for(uint8 i=0; i<SERVE_KEY_MAX; i++)
		{
			if(LockKeyServeKeyList.KeyList[i].Type == KEY_TYPE_TIME_SLOT_7)
			{
			    offset = j;
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
				}
				memset(&LockKeyServeKeyList.KeyList[i], 0xff, sizeof(LockKey_ServeKeyType));
			}
		}
	}
	BlackList_StorePara();
	
	ServeKeyList_Writeflash( offset );	
	//ServeKeyList_StorePara1();//
	return E_OK;
}

/****************************************************************************************************
**Function:
	uint8 LockKey_Add_CardKey(uint8 *pData)
**Author: rory
**Description:
**Input: 
**Output: 
****************************************************************************************************/
uint8 LockKey_Start_LearnCardKey(uint8 *pData)
{
	Access_UserId_Temp = BUILD_UINT32(pData[3],pData[2],pData[1],pData[0]);
	memcpy(Access_AttTIme_Temp, pData+12, 4); /* 开始时间*/
	memcpy(&Access_AttTIme_Temp[4], pData+8, 4); /* 结束j时间*/
        Access_AttTIme_Temp[8] = pData[16];
	Sys_PataCfg.State = WORK_LEARN_CARD;
	Access_CardLearnTimer64 = 30000/64;
	BLE_UPDATA_DISCONNECT_TIMER(30000/64);
	return E_OK;
}
/****************************************************************************************************
**Function:
	uint8 LockKey_Add_CardKey(uint8 *pData)
**Author: rory
**Description:
**Input: 
**Output: 
****************************************************************************************************/
uint8 LockKey_Start_LearnCardKeyExt(uint8 *pData)
{
	Access_UserId_Temp = BUILD_UINT32(pData[3],pData[2],pData[1],pData[0]);
	memcpy(Access_AttTIme_Temp, pData+8, 4); /* 开始时间*/
	memcpy(&Access_AttTIme_Temp[4], pData+4, 4); /* 结束j时间*/
	Sys_PataCfg.State = WORK_LEARN_CARD;
	Access_CardLearnTimer64 = 30000/64;
	return E_OK;
}

/****************************************************************************************************
**Function:
	uint8 LockKey_Add_BlueKey(BlueKey_AllType BlueKey)
**Author: rory
**Description:
**Input: 
**Output: 
****************************************************************************************************/
uint8 LockKey_Remove_Key(uint8 *pData, uint8 type)
{
	uint8 i;
	uint32 userId = BUILD_UINT32(pData[3], pData[2], pData[1], pData[0]);

		if(LockKeyStatistics.ServKeyCount == 0)
		{
			return E_OK;
		}
	    for (uint8 j=0;j<SERVE_KEY_PAGEMAX;j++ )
	    {
		    ServeKeyList_Readflash( j );			
			for(i=0; i<SERVE_KEY_MAX; i++)
			{
				if(LockKeyServeKeyList.KeyList[i].KeyId == userId)
				{
					uint8 year, month, day;
					uint32 tempTime;
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
					memset(&LockKeyServeKeyList.KeyList[i], 0xff, sizeof(LockKey_ServeKeyType));
					if(LockKeyStatistics.ServKeyCount > 0)
					{
						LockKeyStatistics.ServKeyCount--;
					}
					
					ServeKeyList_Writeflash( j );
					//ServeKeyList_StorePara1();
					return E_OK;
				}
			}
	    }
		/* 密码为未使用*/
		BlackList_Add(type, userId,
					BUILD_TIME(ProtoAnaly_Rtcinfo.Year,ProtoAnaly_Rtcinfo.Month,ProtoAnaly_Rtcinfo.Day,
					23,59,59));
		BlackList_StorePara();
	return E_OK;
}

/****************************************************************************************************
**Function:
	void BlackList_LoadPara(void)
**Author: lxw
**Description:
**Input: 
**Output: 
****************************************************************************************************/
void BlackList_Add(uint8 Type, uint32 KeyId, uint32 InvalidTime)
{
	uint8 index = 0xff;

	for(uint8 i=0; i<BLACK_LIST_MAX; i++)
	{
		if(LockKeyBlackList.BlackList[i].KeyId == KeyId)
		{
			LockKeyBlackList.BlackList[i].Type = Type;
			LockKeyBlackList.BlackList[i].KeyId = KeyId;
			LockKeyBlackList.BlackList[i].InvalidTime = InvalidTime;
			return;
		}
		else if(index == 0xff)
		{
			if(LockKeyBlackList.BlackList[i].KeyId == 0xffffffff)
			{
				index = i;
			}
		}
	}
	if(index != 0xff)
	{
		LockKeyBlackList.BlackList[index].Type = Type;
		LockKeyBlackList.BlackList[index].KeyId = KeyId;
		LockKeyBlackList.BlackList[index].InvalidTime = InvalidTime;
		LockKeyStatistics.BlackListCount++;
	}
}


/****************************************************************************************************
**Function:
	void LockKey_CheckRemoveBlackList(uint8 index)
**Author: rory
**Description: 返回id所在色索引 
**Input: 
**Output: 
****************************************************************************************************/
uint8 BlackList_Remove(uint8 index)
{	
	if(LockKeyBlackList.BlackList[index].InvalidTime < ProtoAnaly_RtcLocalTime)
	{
		memset(&LockKeyBlackList.BlackList[index], 0xff, sizeof(LockKey_BlackType));
		if(LockKeyStatistics.BlackListCount > 0)
		{
			LockKeyStatistics.BlackListCount--;
		}
		BlackList_StorePara();
		return E_OK;
	}
	return E_NOT_OK;
}


/****************************************************************************************************
**Function:
	void BlackList_LoadPara(void)
**Author: lxw
**Description:
**Input: 
**Output: 
****************************************************************************************************/
void BlackList_LoadPara(void)
{
#if ((defined USE_FLASH_NRF) && (USE_FLASH_NRF == STD_TRUE))
	uint32 crcdat;
	NrfFlash_PstorageType Message;
	Message.Sector = NRF_FLASH_BLACK_LIST;
	Message.pData= (uint8_t *)&LockKeyBlackList;
	Message.Add = 0;
	Message.Lenth = sizeof(LockKey_BlackListType);
	if(NrfFlashDrive.read(&Message) == E_OK)
	{
		crcdat = LockKeyBlackList.KeyWord; 
		LockKeyBlackList.KeyWord = 0;
		if(crcdat != crc32_compute((uint8_t *)&LockKeyBlackList, sizeof(LockKey_BlackListType), NULL))
		{
			Message.Sector = NRF_FLASH_BLACK_LISTBK;
			NrfFlashDrive.read(&Message);
			crcdat = LockKeyBlackList.KeyWord; 
			Sys_PataCfg.KeyWord = 0;
			if(crcdat != crc32_compute((uint8_t *)&LockKeyBlackList, sizeof(LockKey_BlackListType), NULL))
			{	
				memset((uint8_t *)&LockKeyBlackList, 0xff, sizeof(LockKey_BlackListType));
				BlackList_StorePara();
			}
			else
			{
				LockKeyBlackList.KeyWord = crcdat;
			}
		}
		else
		{
			LockKeyBlackList.KeyWord = crcdat;
		}
	}
	else
	{
		NVIC_SystemReset();
	}
#else
    FlashDrive.open(NULL); 
	Flash_ComType eepdata;
	eepdata.pData = (uint8_t *)&LockKeyBlackList;
	eepdata.Lenth = sizeof(LockKeyBlackList);
	eepdata.Add = ACCESS_BLACK_LIST_OFFSET;
	FlashDrive.read(&eepdata);
	if(LockKeyBlackList.KeyWord!= 0x12345678)
	{
		memset(&LockKeyBlackList,0xff,sizeof(LockKeyBlackList));
		BlackList_StorePara();
	}
#endif
}

/*
void BlackList_Readflash( uint8 offset )
{
	FlashDrive.open(NULL); 

	Flash_ComType eepdata;
	eepdata.pData = (uint8_t *)&LockKeyBlackList;
	eepdata.Lenth = sizeof(LockKeyBlackList);
	eepdata.Add = ACCESS_BLACK_LIST_OFFSET + offset<<12;    
	FlashDrive.read(&eepdata);
}
*/


/****************************************************************************************************
**Function:
	void BlackList_StorePara(void)
**Author: lxw
**Description:
**Input: 
**Output: 
****************************************************************************************************/
void BlackList_StorePara(void)
{
#if ((defined USE_FLASH_NRF) && (USE_FLASH_NRF == STD_TRUE))
	NrfFlash_PstorageType Message;

	if(NrfFlash_Store(NRF_FLASH_BLACK_LIST) == E_OK)
	{
		Message.Sector = NRF_FLASH_BLACK_LISTBK;
	}
	else
	{
		Message.Sector = NRF_FLASH_BLACK_LIST;
	}
	Message.pData= NULL;
	Message.Add = 0;
	Message.Lenth = 0;
	NrfFlashDrive.ioctl(NRFFLASH_STORE, &Message);
#else
	FlashDrive.open(NULL); 

	Flash_ComType eepdata;

	LockKeyBlackList.KeyWord = 0x12345678;
	eepdata.pData = (uint8_t *)&LockKeyBlackList;
	eepdata.Lenth = sizeof(LockKeyBlackList);
	eepdata.Add = ACCESS_BLACK_LIST_OFFSET;
	FlashDrive.ioctl(FLASH_ERASE_SECT,&eepdata.Add);
	FlashDrive.write(&eepdata);
#endif
}


/****************************************************************************************************
**Function:
	void BlackList_RemoveKey(uint8 idtpye, uint8 pData)
**Author: lxw
**Description:
**Input: 
**Output: 
****************************************************************************************************/
void BlackList_RemoveKey(uint8 idtpye, uint32 keyId)
{
	uint8 modify = 0;
	
	for(uint8 i=0; i<BLACK_LIST_MAX; i++)
	{
		if(idtpye == LockKeyBlackList.BlackList[i].Type)
		{
			if(keyId == LockKeyBlackList.BlackList[i].KeyId)
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
}


/****************************************************************************************************
**Function:
	void ServeKeyList_LoadPara(void)
**Author: lxw
**Description:
**Input: 
**Output: 
****************************************************************************************************/
void ServeKeyList_LoadPara(void)
{
#if ((defined USE_FLASH_NRF) && (USE_FLASH_NRF == STD_TRUE))
	uint32 crcdat;
	NrfFlash_PstorageType Message;
	Message.Sector = NRF_FLASH_SERVE_KEY;
	Message.pData= (uint8_t *)&LockKeyServeKeyList;
	Message.Add = 0;
	Message.Lenth = sizeof(LockKey_ServeKeyListType);
	if(NrfFlashDrive.read(&Message) == E_OK)
	{
		crcdat = LockKeyServeKeyList.KeyWord; 
		LockKeyServeKeyList.KeyWord = 0;
		if(crcdat != crc32_compute((uint8_t *)&LockKeyServeKeyList, sizeof(LockKey_ServeKeyListType), NULL))
		{
			Message.Sector = NRF_FLASH_SERVE_KEYBK;
			NrfFlashDrive.read(&Message);
			crcdat = LockKeyServeKeyList.KeyWord; 
			LockKeyServeKeyList.KeyWord = 0;
			if(crcdat != crc32_compute((uint8_t *)&LockKeyServeKeyList, sizeof(LockKey_ServeKeyListType), NULL))
			{
				memset((uint8_t *)&LockKeyServeKeyList, 0xff, sizeof(LockKey_ServeKeyListType));
				ServeKeyList_StorePara();
			}
			else
			{
				LockKeyServeKeyList.KeyWord = crcdat;
			}
		}
		else
		{
			LockKeyServeKeyList.KeyWord = crcdat;
		}
	}
	else
	{
		NVIC_SystemReset();
	}
#else
/*
	FlashDrive.open(NULL); 

	Flash_ComType eepdata;
	uint8 i;
	eepdata.pData = (uint8_t *)&LockKeyServeKeyList;
	eepdata.Lenth = sizeof(LockKeyServeKeyList);
	eepdata.Add = ACCESS_SERVE_KEY_OFFSET;
	FlashDrive.read(&eepdata);
*/	
	//if(LockKeyServeKeyList.KeyWord!= 0x12345678)
	//{
	//	memset(&LockKeyServeKeyList,0xff,sizeof(LockKeyServeKeyList));
	//	ServeKeyList_StorePara1();
	//}
#endif
}

void ServeKeyList_Readflash( uint8 offset )
{
	FlashDrive.open(NULL); 

	Flash_ComType eepdata;
	eepdata.pData = (uint8_t *)&LockKeyServeKeyList;
	eepdata.Lenth = sizeof(LockKeyServeKeyList);
	eepdata.Add = ACCESS_SERVE_KEY_OFFSET + offset<<12;    // 
	FlashDrive.read(&eepdata);
}

/****************************************************************************************************
**Function:
	void ServeKeyList_StorePara(void)
**Author: lxw
**Description:
**Input: 
**Output: 
****************************************************************************************************/
void ServeKeyList_StorePara(void)
{
#if ((defined USE_FLASH_NRF) && (USE_FLASH_NRF == STD_TRUE))
	NrfFlash_PstorageType Message;

	if(NrfFlash_Store(NRF_FLASH_SERVE_KEY) == E_OK)
	{
		Message.Sector = NRF_FLASH_SERVE_KEYBK;
	}
	else
	{
		Message.Sector = NRF_FLASH_SERVE_KEY;
	}
	Message.pData= NULL;
	Message.Add = 0;
	Message.Lenth = 0;
	NrfFlashDrive.ioctl(NRFFLASH_STORE, &Message);
#else
/*
	FlashDrive.open(NULL); 

	Flash_ComType eepdata;
	
	LockKeyServeKeyList.KeyWord = 0x12345678;
	eepdata.pData = (uint8_t *)&LockKeyServeKeyList;
	eepdata.Lenth = sizeof(LockKeyServeKeyList);
	eepdata.Add = ACCESS_SERVE_KEY_OFFSET;
	FlashDrive.ioctl(FLASH_ERASE_SECT,&eepdata.Add);
	FlashDrive.write(&eepdata);
	*/
#endif
}
    


void ServeKeyList_Writeflash(uint8 offset)
{
	FlashDrive.open(NULL); 
	Flash_ComType eepdata;
	
	eepdata.pData = (uint8_t *)&LockKeyServeKeyList;
	eepdata.Lenth = sizeof(LockKeyServeKeyList);
	eepdata.Add = ACCESS_SERVE_KEY_OFFSET +offset<<12;
	FlashDrive.ioctl(FLASH_ERASE_SECT,&eepdata.Add);
	FlashDrive.write(&eepdata);
}



/****************************************************************************************************
**Function:
	void ServeKeyList_Remove(uint8 index)
**Author: rory
**Description: 
**Input: 
**Output: 
****************************************************************************************************/
uint8 ServeKeyList_Remove(uint8 index)
{	
    ServeKeyList_Readflash(ServeKeyFlashOffset);
	if(LockKeyServeKeyList.KeyList[index].EndTime < ProtoAnaly_RtcLocalTime)
	{
		memset(&LockKeyServeKeyList.KeyList[index], 0xff, sizeof(LockKey_ServeKeyType));
		if(LockKeyStatistics.ServKeyCount > 0)
		{
			LockKeyStatistics.ServKeyCount--;
		}
		ServeKeyList_Writeflash(ServeKeyFlashOffset);
		//ServeKeyList_StorePara1();
		return E_OK;
	}
	return E_NOT_OK;
}

/****************************************************************************************************
**Function:
	void RecordList_CheckRtc(void)
**Author: rory
**Description:
**Input: 
**Output: 
****************************************************************************************************/
void RecordList_CheckRtc(void)
{
	Rtc_Type Rtc_data;
	Rtc_Type Rtc_data_flash;
		
	if(Rtc_Read(&Rtc_data)== E_OK)
	{
		Rtc_data_flash.Year = BUILD_YEAR(AccRcord.TimerBk);
		Rtc_data_flash.Month = BUILD_MONTH(AccRcord.TimerBk);
		Rtc_data_flash.Day = BUILD_DAY(AccRcord.TimerBk);
		Rtc_data_flash.Hour = BUILD_HOUR(AccRcord.TimerBk);
		Rtc_data_flash.Minute = BUILD_MINUTE(AccRcord.TimerBk);
		Rtc_data_flash.Second = BUILD_SECOND(AccRcord.TimerBk);

		if((Rtc_data_flash.Year > 30) || (Rtc_data_flash.Month > 12) || (Rtc_data_flash.Day > 31)
			|| (Rtc_data_flash.Hour > 24) || (Rtc_data_flash.Year < 18))
		{
			return;
		}
		if(Rtc_data.Year < 18)
		{
			memcpy(&Rtc_data, &Rtc_data_flash, sizeof(Rtc_Type));
			
			Rtc_Ioctl(RTC_CLOCKSET,&Rtc_data);
			ProtoAnaly_RtcLocalTime = AccRcord.TimerBk;
		}
	}
}

/****************************************************************************************************
**Function:
	void RecordList_LoadPara(void)
**Author: lxw
**Description:
**Input: 
**Output: 
****************************************************************************************************/
void RecordList_LoadPara(void)
{
#if ((defined USE_FLASH_NRF) && (USE_FLASH_NRF == STD_TRUE))
	uint32 crcdat;
	NrfFlash_PstorageType Message;
	Message.Sector = NRF_FLASH_RECORD;
	Message.pData= (uint8_t *)&AccRcord;
	Message.Add = 0;
	Message.Lenth = sizeof(AccRcordParaType);
	if(NrfFlashDrive.read(&Message) == E_OK)
	{
		crcdat = AccRcord.KeyWord; 
		AccRcord.KeyWord = 0;
		if(crcdat != crc32_compute((uint8_t *)&AccRcord, sizeof(AccRcordParaType), NULL))
		{
			memset((uint8_t *)&AccRcord, 0xff, sizeof(AccRcordParaType));
			AccRcord.Wrecordindex = 0;
			AccRcord.Srecordindex = 0;
			AccRcord.recordnum = 0;
			AccRcord.TimerBk = 0;
			AccRcord.CustomTimerBk = 0;
			AccRcord.StaffTimerBk = 0;
			AccRcord.AllTimerBk = 0;
			AccRcord.EmergencyTimerBk = 0;
			RecordList_StorePara(1);
		}
		else
		{
			AccRcord.KeyWord = crcdat;
			if(AccRcord.TimerBk == 0xffffffff)
			{
				AccRcord.TimerBk = 0;
			}
			if(AccRcord.CustomTimerBk == 0xffffffff)
			{
				AccRcord.CustomTimerBk = 0;
			}
			if(AccRcord.StaffTimerBk == 0xffffffff)
			{
				AccRcord.StaffTimerBk = 0;
			}
			if(AccRcord.AllTimerBk == 0xffffffff)
			{
				AccRcord.AllTimerBk = 0;
			}
			if(AccRcord.EmergencyTimerBk == 0xffffffff)
			{
				AccRcord.EmergencyTimerBk = 0;
			}
			if(AccRcord.AppTimerBk == 0xffffffff)
			{
				AccRcord.AppTimerBk = 0;
			}
			//RecordList_CheckRtc();
		}
	}
#else
		Flash_ComType eepdata;
		uint8 i;
		
		FlashDrive.open(NULL); 
		eepdata.pData = (uint8_t *)&AccRcord;
		eepdata.Lenth = sizeof(AccRcord);
		eepdata.Add = ACCESS_RECORD_OFFSET;
		FlashDrive.read(&eepdata);
		
		if(AccRcord.TimerBk == 0xffffffff)
		{
			AccRcord.TimerBk = 0;
		}
		if(AccRcord.CustomTimerBk == 0xffffffff)
		{
			AccRcord.CustomTimerBk = 0;
		}
		if(AccRcord.StaffTimerBk == 0xffffffff)
		{
			AccRcord.StaffTimerBk = 0;
		}
		if(AccRcord.AllTimerBk == 0xffffffff)
		{
			AccRcord.AllTimerBk = 0;
		}
		if(AccRcord.EmergencyTimerBk == 0xffffffff)
		{
			AccRcord.EmergencyTimerBk = 0;
		}
		if(AccRcord.AppTimerBk == 0xffffffff)
		{
			AccRcord.AppTimerBk = 0;
		}
#endif
}

/****************************************************************************************************
**Function:
	void RecordList_StorePara(void)
**Author: lxw
**Description:
**Input: mode:1  直接存储  0:延时存储
**Output: 
****************************************************************************************************/
void RecordList_StorePara(uint8 mode)
{
#if ((defined USE_FLASH_NRF) && (USE_FLASH_NRF == STD_TRUE))
	//NrfFlash_Store(NRF_FLASH_RECORD);
	NrfFlash_PstorageType Message;
	if(mode == 1)
	{
		Message.Sector = NRF_FLASH_RECORD;
		NrfFlashDrive.ioctl(NRFFLASH_STORE_DIR, &Message);
	}
	else
	{
	Message.Sector = NRF_FLASH_RECORD;
	NrfFlashDrive.ioctl(NRFFLASH_STORE, &Message);
	}
#else
	FlashDrive.open(NULL); 

	Flash_ComType eepdata;

	Sys_PataCfg.KeyWord = 0x12345678;
	eepdata.pData = (uint8_t *)&AccRcord;
	eepdata.Lenth = sizeof(AccRcord);
	eepdata.Add = ACCESS_RECORD_OFFSET;
	FlashDrive.ioctl(FLASH_ERASE_SECT,&eepdata.Add);
	FlashDrive.write(&eepdata);
#endif


}

