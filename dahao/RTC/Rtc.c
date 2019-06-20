/*****************************************************************************
|  File Name: Rtc.c
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

#include "RTC\Rtc.h"
#include"I2C\I2c_Soft.h"
#include "Access\Access.h"
#include "SYSTEM\sys.h"
#include "protocol\Proto_Analy.h"

uint8 Rtc_Wlarm_Enable;
/****************************************************************************************************
**Function:
Std_ReturnType Rtc_ReadNBytes(Rtc_CommType Cmd, BYTE *pDataBuf, uint8 NumBytes)
**Author: lxw
**Description:
**Input: 
**Output: 
****************************************************************************************************/
Std_ReturnType Rtc_ReadNBytes(Rtc_CommType Cmd, BYTE *pDataBuf, uint8 NumBytes)
{
	if (0 < NumBytes)
	{
		if ((RTC_SUCCESS == I2C_SOFT_Start())&& (RTC_SUCCESS == I2C_SOFT_Byte_Tx((BYTE)Cmd | 0x01)))
		{
			while (1 < NumBytes--)
			{
				if (RTC_SUCCESS != I2C_SOFT_Byte_Rx(pDataBuf, I2C_SOFT_RX_NOT_LAST_BYTE))
				{
					I2C_SOFT_Stop();
					return RTC_FAILED;
				}
				++pDataBuf;
			}
			if (RTC_SUCCESS == I2C_SOFT_Byte_Rx(pDataBuf, I2C_SOFT_RX_LAST_BYTE))
			{
				I2C_SOFT_Stop();

				return RTC_SUCCESS;
			}
		}
		I2C_SOFT_Stop();
		return RTC_FAILED;
	}
	return RTC_SUCCESS;
}
/****************************************************************************************************
**Function:
Std_ReturnType Rtc_WriteNBytes(Rtc_CommType Cmd, BYTE *pDataBuf, uint16 NumBytes)
**Author: lxw
**Description:
**Input: 
**Output: 
****************************************************************************************************/
Std_ReturnType Rtc_WriteNBytes(Rtc_CommType Cmd, BYTE *pDataBuf, uint16 NumBytes)
{
	if((RTC_SUCCESS == I2C_SOFT_Start())&& (RTC_SUCCESS == I2C_SOFT_Byte_Tx((BYTE)Cmd)))
	{
		while (0 < NumBytes--)
		{
			if (RTC_SUCCESS != I2C_SOFT_Byte_Tx(*pDataBuf++))
			{
				I2C_SOFT_Stop();
				return RTC_FAILED;
			}
		}
		I2C_SOFT_Stop();
		return RTC_SUCCESS;
	}
	I2C_SOFT_Stop();
	return RTC_FAILED;
}
/****************************************************************************************************
**Function:
void Rtc_Init(void)
**Author: lxw
**Description:
**Input: 
**Output: 
****************************************************************************************************/
void Rtc_Init(void)
{
	uint8 DataBuffer[2];
	I2C_SOFT_Init();
	DataBuffer[0] = 0x40;
	Rtc_WriteNBytes(RTC_ADD|RTC_COMM_STATUS_REG1,DataBuffer,1); 
	
	DataBuffer[0] = 0;
	Rtc_WriteNBytes(RTC_ADD|RTC_COMM_STATUS_REG2,DataBuffer,1);
}
/****************************************************************************************************
**Function:
   uint8 Rtc_SwapByte(uint8 Data)
**Author: lxw
**Description:
**Input: 
**Output: 
****************************************************************************************************/
uint8 Rtc_SwapByte(uint8 Data)
{
	uint8 tempdata ;
	tempdata = 0;
	tempdata |= (0x01&Data)<<7;
	tempdata |= (0x02&Data)<<5;
	tempdata |= (0x04&Data)<<3;
	tempdata |= (0x08&Data)<<1;
	tempdata |= (0x10&Data)>>1;
	tempdata |= (0x20&Data)>>3;
	tempdata |= (0x40&Data)>>5;
	tempdata |= (0x80&Data)>>7;
	return (tempdata);
}

/******************************************************************************
************************Function:
uint8 Main_GetWeekFromDay(unsigned char year,unsigned char month,unsigned 
char day) 
**Author: lxw
**Description:
**Input: 
**Output: 
*******************************************************************************
*********************/
// 计算2000～2099年任一天星期几 
// year    : 00-99 
// month: 01-12 
// day     : 01-31 
uint8 Main_GetWeekFromDay(unsigned char year,unsigned char month,unsigned char day) 
{ 
    if( month == 1 || month == 2 )   
    { 
        month += 12; 
        if( year > 0 ) 
            year--; 
        else 
            year = 4; 
    } 

    // 返回星期几(星期一用1表示，而星期天用7表示) 
    return 1+(( day + 2*month + 3*(month+1)/5 + year + year/4 ) %7); 
}


/****************************************************************************************************
**Function:
   void  Rtc_Set(Rtc_Type *pData)
**Author: lxw
**Description:
**Input: 
**Output: 
****************************************************************************************************/
void  Rtc_Set(Rtc_Type *pData)
{
	uint8 DataBuffer[8];
	Rtc_Init();

	pData->Week = Main_GetWeekFromDay(pData->Year,pData->Month,pData->Day);
	DataBuffer[0] = Rtc_SwapByte(((pData->Year/10)<<4)|(pData->Year%10)); 
	DataBuffer[1] = Rtc_SwapByte(((pData->Month/10)<<4)|(pData->Month%10));//(pData->Month); 
	DataBuffer[2] = Rtc_SwapByte(((pData->Day/10)<<4)|(pData->Day%10));//(pData->Day); 
	DataBuffer[3] = Rtc_SwapByte(((pData->Week/10)<<4)|(pData->Week%10));; 
	DataBuffer[4] = Rtc_SwapByte(((pData->Hour/10)<<4)|(pData->Hour%10));//(pData->Hour); 
	DataBuffer[5] = Rtc_SwapByte(((pData->Minute/10)<<4)|(pData->Minute%10));//(pData->Minute);
	DataBuffer[6] = Rtc_SwapByte(((pData->Second/10)<<4)|(pData->Second%10));//(pData->Second);
	Rtc_WriteNBytes(RTC_ADD|RTC_COMM_ALL_REALTIME,DataBuffer,7);
	
	I2c_SoftClockSetOutput();
	I2c_SoftDatSetOutput();
	I2c_SoftClockhigh();
	I2c_SoftDathigh();
	memset(DataBuffer,0,sizeof(DataBuffer));
}
/****************************************************************************************************
**Function:
   void  Rtc_Set(Rtc_Type *pData)
**Author: lxw
**Description:
**Input: 
**Output: 
****************************************************************************************************/
void  Rtc_WlarmSet(Rtc_WlarmType *pData)
{
	uint8 DataBuffer[3];
	Rtc_Init();
	Rtc_ReadNBytes(RTC_ADD|RTC_COMM_STATUS_REG1,DataBuffer,1);
	if(pData->Flag == STD_TRUE)
	{
		DataBuffer[0] = 0x20;
		Rtc_WriteNBytes(RTC_ADD|RTC_COMM_STATUS_REG2,DataBuffer,1);
		DataBuffer[0] = 0;//Rtc_SwapByte(((pData->Week/10)<<4)|(pData->Week%10)) ;
		DataBuffer[1] = Rtc_SwapByte(((pData->Hour/10)<<4)|(pData->Hour%10))| 0x01; //(pData->Hour); 
		DataBuffer[2] = Rtc_SwapByte(((pData->Minute/10)<<4)|(pData->Minute%10))| 0x01; //(pData->Minute);
		Rtc_WriteNBytes(RTC_ADD|RTC_COMM_INT1,DataBuffer,3);

	}
	else if(pData->Flag == STD_FALSE)
	{
		DataBuffer[0] = 0;
		Rtc_WriteNBytes(RTC_ADD|RTC_COMM_STATUS_REG2,DataBuffer,1);
	}
	else 
	{
		DataBuffer[0] = 0x0;
		Rtc_WriteNBytes(RTC_ADD|RTC_COMM_STATUS_REG2,DataBuffer,1);
	}
}
/****************************************************************************************************
**Function:
   int Rtc_Open(void *pData)
**Author: lxw
**Description:
**Input: 
**Output: 
****************************************************************************************************/
void Rtc_Open(void *pData)
{
	Rtc_Type Rtc_data;
	
	Rtc_Init();
	RtcDrive.read(&Rtc_data);
	if((Rtc_data.Year == 0) ||(Rtc_data.Year > 99) ||(Rtc_data.Year < 18))
	{
		Rtc_data.Year = BUILD_YEAR(AccRcord.TimerBk);
		Rtc_data.Month = BUILD_MONTH(AccRcord.TimerBk);
		Rtc_data.Day = BUILD_DAY(AccRcord.TimerBk);
		Rtc_data.Hour = BUILD_HOUR(AccRcord.TimerBk);
		Rtc_data.Minute = BUILD_MINUTE(AccRcord.TimerBk);
		Rtc_data.Second = BUILD_SECOND(AccRcord.TimerBk);
		RtcDrive.ioctl(RTC_CLOCKSET,&Rtc_data);
		ProtoAnaly_RtcLocalTime = AccRcord.TimerBk;
		
		Vdd_PowerOn();
		Access_LockTimer64 = Sys_PataCfg.OpenLockTime/64;
		Access_BeepStart(BEEP_NORMAL,1);
	}
	I2c_SoftClockSetOutput();
	I2c_SoftDatSetOutput();
	I2c_SoftClockhigh();
	I2c_SoftDathigh();
}
/****************************************************************************************************
**Function:
   int8 Rtc_Read(void *pData)
**Author: lxw
**Description:
**Input: 
**Output: 
****************************************************************************************************/
Std_ReturnType Rtc_Read(void *pData)
{
	uint8 DataBuffer[8];
	if(Rtc_ReadNBytes(RTC_ADD|RTC_COMM_ALL_REALTIME,DataBuffer,7) == RTC_SUCCESS)
	{
		((Rtc_Type *)pData)->Year = ((Rtc_SwapByte(DataBuffer[0])&0xf0)>>4)*10 + (Rtc_SwapByte(DataBuffer[0])&0x0f);
		((Rtc_Type *)pData)->Month =((Rtc_SwapByte(DataBuffer[1])&0xf0)>>4)*10 + (Rtc_SwapByte(DataBuffer[1])&0x0f);
		((Rtc_Type *)pData)->Day = ((Rtc_SwapByte(DataBuffer[2])&0xf0)>>4)*10 + (Rtc_SwapByte(DataBuffer[2])&0x0f);
		((Rtc_Type *)pData)->Week = ((Rtc_SwapByte(DataBuffer[3])&0xf0)>>4)*10 + (Rtc_SwapByte(DataBuffer[3])&0x0f);
		((Rtc_Type *)pData)->Hour =((Rtc_SwapByte(DataBuffer[4])&0x30)>>4)*10 + (Rtc_SwapByte(DataBuffer[4])&0x0f);
		((Rtc_Type *)pData)->Minute = ((Rtc_SwapByte(DataBuffer[5])&0xf0)>>4)*10 + (Rtc_SwapByte(DataBuffer[5])&0x0f);
		((Rtc_Type *)pData)->Second = ((Rtc_SwapByte(DataBuffer[6])&0xf0)>>4)*10 + (Rtc_SwapByte(DataBuffer[6])&0x0f);

		I2c_SoftClockSetOutput();
		I2c_SoftDatSetOutput();
		I2c_SoftClockhigh();
		I2c_SoftDathigh();

		return E_OK;
	}
	I2c_SoftClockSetOutput();
	I2c_SoftDatSetOutput();
	I2c_SoftClockhigh();
	I2c_SoftDathigh();
	return E_NOT_OK;
}
/****************************************************************************************************
**Function:
   Std_ReturnType Rtc_Ioctl(Rtc_CmdType Cmd,void *pData)
**Author: lxw
**Description:
**Input: 
**Output: 
****************************************************************************************************/
Std_ReturnType Rtc_Ioctl(uint8 Cmd,void *pData)
{
	Std_ReturnType Err;
	Err = E_INPAR_ERR;
	if(Cmd < SETRTCCONTROLMAX )
	{
		switch(Cmd)
		{
			case RTC_CLOCKSET:
			{
				Rtc_Set(pData);
				Err = E_OK;
				RecordList_StorePara(0); /* 存储备份时间*/
			}break;
			case RTC_WLARMSET:
			{
				Rtc_WlarmSet(pData);
			}break;
			default:break; 
		}
	}
	return Err;
}

const Dr_OpertonType RtcDrive=
{
	Rtc_Open,NULL, Rtc_Read, NULL,Rtc_Ioctl,
};
#if(RTC_CHECK==	STD_TRUE)
/****************************************************************************************************
**Function:
   int8 ADCUser_Ioctl(void *pData)
**Author: lxw
**Description:
**Input: 
**Output: 
****************************************************************************************************/
void RTC_Check(void)
{
	Rtc_Type Rtc_data;
	Rtc_WlarmType Rtc_WlarmData;
	Rtc_Open(NULL);
	Rtc_data.Year = 13;
	
	Rtc_data.Month = 12;
	Rtc_data.Day = 23;
	Rtc_data.Hour = 5;
	Rtc_data.Minute = 0;
	Rtc_data.Second = 40;
	Rtc_Ioctl(RTC_CLOCKSET,&Rtc_data);

/*	Rtc_WlarmData.Flag = STD_TRUE;
	Rtc_WlarmData.Hour = 5;
	Rtc_WlarmData.Minute= 1;
	RtcDrive.ioctl(RTC_WLARMSET,&Rtc_WlarmData);*/
	for(;;)
	{

		Sys_GenFunDelayMs(200);
		Rtc_Read(&Rtc_data);
//		RtcDrive.ioctl(RTC_CLOCKSET,&Rtc_data);
//		memset(&Rtc_data,0,sizeof(Rtc_data));
	}
}
#endif

