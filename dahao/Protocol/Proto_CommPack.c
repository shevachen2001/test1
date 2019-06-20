/*******************************************************************************************************************
|  File Name: Sys_CommPack.c
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
| 2013-11  1.00                  Initial version 
|******************************************************************************************************************/

#include 	"SYSTEM\Sys_GenFun.h"
#include 	"Protocol\Proto_CommPack.h"
#include 	"Protocol\Proto_NetComm.h"
#include	"Protocol\factInfo.h"
#include	"protocol\Proto_Analy.h"
#include 	"SYSTEM\sys.h"
#include 	"Encpt\Aes.h"
#include 	"Encpt\Encpt.h"
#include 	"COMMU\Lock_Commu.h"
#include 	"ble_dahao.h"
#include 	"Beep\Beep.h"
#include 	"Access\LockKey.h"

static Proto_PackfunCallBack 	Proto_Packfun;
extern Std_ReturnType power_out_sleep(void);
/****************************************************************************************************
**Function:
	void Proto_CommPrackInit(Proto_PackfunCallBack funcalback) 
**Author: LXW
**Description:
**Input: 
**Output: 
****************************************************************************************************/
void Proto_CommPrackInit(Proto_PackfunCallBack *funcalback) 
{
#if(PROTO_PUBLIC == STD_TRUE)
	Proto_Packfun.PublicProcCallBack = funcalback->PublicProcCallBack;
#endif
	Proto_Packfun.MacProcCallBack= funcalback->MacProcCallBack;
	Proto_Packfun.AppProcCallBack = funcalback->AppProcCallBack;
	Proto_Packfun.ProcEndCallBack = funcalback->ProcEndCallBack;
	Proto_Packfun.ProcNetUpdateCallBack= funcalback->ProcNetUpdateCallBack;
	
/*	Proto_Packfun.Tc_SourceAdd1 = funcalback->Tc_SourceAdd1;
	Proto_Packfun.Tc_SourceAdd2 = funcalback->Tc_SourceAdd2;
	Proto_Packfun.Tc_TargeAdd= funcalback->Tc_TargeAdd;*/
}
//static uint8 Proto_PackInbuf[PROTO_COMM_BRG_LENTH];
static uint8 Proto_PackOutbuf[PROTO_COMM_BRG_LENTH];

 uint8 Proto_ttt[PROTO_COMM_BRG_LENTH];
 extern uint8 initFlag;

/****************************************************************************************************
**Function:
	void Proto_CommHostAckPack(Proto_NetcommType nettype,uint8_t* pdatin) 
**Author: LXW
**Description:
**Input: 
**Output: 
****************************************************************************************************/
void Proto_CommHostAckPack(Proto_NetcommType nettype,uint8_t* pdatin) 
{
	uint8 encry[16];
	uint8 checksum;
	uint8 lenth;
	if(pdatin[0] == 0)
	{
		return;
	}
	Sys_GenFun16To8(PROTO_COMMPACK_HOST_DAT_OUT,Proto_PackOutbuf);
	Sys_GenFun16To8(Sys_PataCfg.SysId,(&Proto_PackOutbuf[4]));
	Sys_GenFun32To8(Sys_PataCfg.Mac,(&Proto_PackOutbuf[6]));
	//���ID ��
	pdatin[0] += 1;//
	pdatin[0] += 6;//

	if(nettype == NRF_BLE)
	{
		if((pdatin[0]&0x0f) != 0)
		{
			pdatin[0] = (pdatin[0]&0xF0)+ 0x10;
		}
		//����checksum 
		checksum = Sys_GenFunChecksum(pdatin);
		Proto_PackOutbuf[pdatin[0] + 2] = checksum;
		lenth = pdatin[0]+3;

		
		memcpy(Proto_ttt,Proto_PackOutbuf,PROTO_COMM_BRG_LENTH);

		
		for(checksum= 0;checksum< ((lenth-3)>>4);checksum++)
		{
			AES_Encrypt(&Proto_PackOutbuf[3+(checksum<<4)],encry);
			memcpy(&Proto_PackOutbuf[3+(checksum<<4)],encry,16);
		}
	}
	else if(nettype == GPRS)
	{
		if((pdatin[0]&0x07) != 0)
		{
			pdatin[0] = (pdatin[0]&0xF8)+ 0x08;
		}
		//����checksum 
		checksum = Sys_GenFunChecksum(pdatin);
		Proto_PackOutbuf[pdatin[0] + 2] = checksum;
		lenth = pdatin[0]+3;
		for(checksum= 0;checksum< ((lenth-3)>>3);checksum++)
		{
			memcpy(encry,&Proto_PackOutbuf[3+(checksum<<4)],8);
			Encpt_EncryptExt(encry);
			memcpy(&Proto_PackOutbuf[3+(checksum<<4)],encry,8);
//			Encpt_EncryptExt(&Proto_PackOutbuf[3+(checksum<<3)]);
		}
	}
	else 
	{
#if(COMMENCPT_ENABLE == STD_TRUE)
	if((nettype == TCPIP_CLI)||(nettype == WIFI))
	{
		if((pdatin[0]&0x07) != 0)
		{
			pdatin[0] = (pdatin[0]&0xF8)+ 0x08;
		}
		//����checksum 
		checksum = Sys_GenFunChecksum(pdatin);
		Proto_PackOutbuf[pdatin[0] + 2] = checksum;
		lenth = pdatin[0]+3;
		for(checksum= 0;checksum< ((lenth-3)>>3);checksum++)
		{
			Encpt_EncryptExt(&Proto_PackOutbuf[3+(checksum<<3)]);
		}
	}
	else 
	{
		//����checksum 
		checksum = Sys_GenFunChecksum(pdatin);
		Proto_PackOutbuf[pdatin[0] + 2] = checksum;
		lenth = pdatin[0]+3;
	}
#else 
	//����checksum 
	checksum = Sys_GenFunChecksum(pdatin);
	Proto_PackOutbuf[pdatin[0] + 2] = checksum;
	lenth = pdatin[0]+3;
#endif
	}
	Proto_NetCommSend(nettype,Proto_PackOutbuf,lenth);
}
/****************************************************************************************************
**Function:
	void Proto_CommHostUnPackRec(Proto_NetcommType nettype,uint8* pindat,uint16 lenth) 
**Author: LXW
**Description:
**Input: 
**Output: 
****************************************************************************************************/
void Proto_CommHostUnPackRec(Proto_NetcommType nettype,uint8* pindat,uint16 lenth) 
{
 	uint8 checksum;
	uint8 result;
	
	Proto_Packfun.ProcNetUpdateCallBack(nettype);
#if(PROTO_PUBLIC == STD_TRUE)             //   ProtoAnaly_PublicProc
	if(Proto_Packfun.PublicProcCallBack(nettype,pindat,lenth,Proto_PackOutbuf)== E_OK)
	{
		return ;
	}
#endif
	if(PROTO_COMMPACK_HOST_DAT_IN != Sys_GenFun8To16(pindat))   //  ��ͷ
	{
		return ;
	}

	
#if   (COMMENCPT_ENABLE == STD_TRUE)
	if((nettype != TCPIP_CLI)&&(nettype != GPRS)&&(nettype != WIFI))
	{
		checksum = pindat[pindat[PROTO_COMM_COMD_LENTHADD]+2];
		if(checksum != Sys_GenFunChecksum(&pindat[PROTO_COMM_COMD_LENTHADD]))
		{
			return ;
		}
	}
	else 
	{
		for(checksum= 0;checksum< ((lenth-3)>>3);checksum++)
		{
			Encpt_DecrpytExt(&pindat[3+(checksum<<3)]);
		}
		checksum = pindat[pindat[PROTO_COMM_COMD_LENTHADD]+2];
		if(checksum != Sys_GenFunChecksum(&pindat[PROTO_COMM_COMD_LENTHADD]))
		{
			return ;
		}
	}
#else 
	checksum = pindat[pindat[PROTO_COMM_COMD_LENTHADD]+2];
	if(checksum != Sys_GenFunChecksum(&pindat[PROTO_COMM_COMD_LENTHADD]))
	{
		return ;
	}
#endif		
		//MAC�㴦��
		result = Proto_Packfun.MacProcCallBack(nettype,&pindat[PROTO_COMM_COMD_OFFSET],&Proto_PackOutbuf[PROTO_COMM_COMD_LENTHADD]);
		if(result == 0)
		{
	/*		if((nettype == Proto_Packfun.Tc_SourceAdd1)||(nettype == Proto_Packfun.Tc_SourceAdd2))
			{
				Proto_NetCommSend(Proto_Packfun.Tc_TargeAdd,pindat,pindat[2]+3);
			}*/
			return ;
		}
		else if(result == 1)
		{
			Proto_CommHostAckPack(nettype,&Proto_PackOutbuf[PROTO_COMM_COMD_LENTHADD]);
			goto PROCEND;
		}
		uint8 recCmd = pindat[PROTO_COMM_COMD_OFFSET+1];

		if((recCmd != COMD_LOCK_UP) && (recCmd != COMD_LOCK_DOWN))
		{
			if((Sys_PataCfg.Mac != Sys_GenFun8To32(&pindat[6]))&&(0xffffffff != Sys_GenFun8To32(&pindat[6])))	
			{	
				if((recCmd != COMD_GET_HEARTBEAT)&&(recCmd != COMD_UPLOAD_RECORD)&&(recCmd != COMD_LARNCARD_OK_ID))
				{
					Proto_PackOutbuf[PROTO_COMM_COMD_LENTHADD] = 3;
					Proto_PackOutbuf[PROTO_COMM_COMD_CMDADD] = recCmd; 
					Proto_PackOutbuf[PROTO_COMM_COMD_CMDADD + 7] = E_LOCK_ID_WRONG;
					Proto_CommHostAckPack(nettype,&Proto_PackOutbuf[PROTO_COMM_COMD_LENTHADD]);
					Access_BeepStart(BEEP_NORMAL, BEEP_ERROR_ID);
					Access_LightStart(LIGHT_NG,LIGHT_SLOW,1,0);
					goto PROCEND;
				}
				return ;
			}
		}
		/* �˳�����ģʽ*/
		if(Sys_PataCfg.Pswd == 0x12345678)
		{
			if(Sys_PataCfg.Mac != Sys_GenFun8To32(&pindat[6]))	
			{	
				Proto_PackOutbuf[PROTO_COMM_COMD_LENTHADD] = 3;
				Proto_PackOutbuf[PROTO_COMM_COMD_CMDADD] = recCmd; 
				Proto_PackOutbuf[PROTO_COMM_COMD_CMDADD + 7] = E_LOCK_ID_WRONG;
				Proto_CommHostAckPack(nettype,&Proto_PackOutbuf[PROTO_COMM_COMD_LENTHADD]);
				Access_BeepStart(BEEP_NORMAL, BEEP_ERROR_ID);
				Access_LightStart(LIGHT_NG,LIGHT_SLOW,1,0);
				goto PROCEND;
			}
			else
			{
			  initFlag = 1;
			  
			//	memcpy(Sys_PataCfg.Aeskey, &pindat[14], 16);
			//	Sys_StorePara();
			//	Access_BeepStart(BEEP_OPEN,1);
			}
			
			
			if(0x12345678 != Sys_GenFun8To32(&pindat[10]))
			{
				Sys_PataCfg.Pswd = Sys_GenFun8To32(&pindat[10]);
				Sys_StorePara();
				Access_EraseAllKey(KEY_TYPE_ALL);
				Access_EraseRecordData();
				memset(&LockKeyStatistics,0,sizeof(LockKeyStatistics));
				ProtoAnalyHashKey = ProtoAnaly_HashKeyInit(Sys_PataCfg.Pswd);
			}
		}
        else
		{
		#if 1
			if((recCmd == COMD_LOCK_UP) || (recCmd == COMD_LOCK_DOWN))
			{ /* Rently request */
				uint8 temp_buf[8];

				memcpy(temp_buf, &pindat[6], 8);
				Encpt_DecrpytExt(temp_buf);

				if(ProtoAnalyHashKey != Sys_GenFun8To32(&temp_buf[0]))
				{
					Sys_GenFun32To8(Sys_PataCfg.Mac, &pindat[6]);
					Proto_PackOutbuf[PROTO_COMM_COMD_LENTHADD] = 3;
					Proto_PackOutbuf[PROTO_COMM_COMD_CMDADD] = recCmd; 
					Proto_PackOutbuf[PROTO_COMM_COMD_CMDADD + 7] = E_USER_PASWD_WRONG;
					Proto_CommHostAckPack(nettype,&Proto_PackOutbuf[PROTO_COMM_COMD_LENTHADD]);
					Access_BeepStart(BEEP_NORMAL, BEEP_ERROR_USR_PASWD);
					Access_LightStart(LIGHT_NG,LIGHT_SLOW,1,0);
					goto PROCEND;
				}
				Sys_GenFun32To8(Sys_PataCfg.Mac, &pindat[6]);
				ProtoAnaly_UpdateTime();
				if(ProtoAnaly_RtcLocalTime > Sys_GenFun8To32(&temp_buf[4]))
				{
					Proto_PackOutbuf[PROTO_COMM_COMD_LENTHADD] = 3;
					Proto_PackOutbuf[PROTO_COMM_COMD_CMDADD] = recCmd; 
					Proto_PackOutbuf[PROTO_COMM_COMD_CMDADD + 7] = E_TIME_OUT;
					Proto_CommHostAckPack(nettype,&Proto_PackOutbuf[PROTO_COMM_COMD_LENTHADD]);
					Access_BeepStart(BEEP_NORMAL, BEEP_ERROR_TIME_OUT);
					Access_LightStart(LIGHT_NG,LIGHT_SLOW,1,0);
					goto PROCEND;
				}
			}
		#endif
		}
		if((recCmd != COMD_LOCK_UP) && (recCmd != COMD_LOCK_DOWN))
		{
			if(Sys_PataCfg.Pswd!= Sys_GenFun8To32(&pindat[10])) 
			{	
				if((recCmd != COMD_GET_HEARTBEAT)&&(recCmd != COMD_UPLOAD_RECORD)&&(recCmd != COMD_LARNCARD_OK_ID))
				{
					Proto_PackOutbuf[PROTO_COMM_COMD_LENTHADD] = 3;
					Proto_PackOutbuf[PROTO_COMM_COMD_CMDADD] = recCmd; 
					Proto_PackOutbuf[PROTO_COMM_COMD_CMDADD + 7] = E_USER_PASWD_WRONG;
					Proto_CommHostAckPack(nettype,&Proto_PackOutbuf[PROTO_COMM_COMD_LENTHADD]);
					Access_BeepStart(BEEP_NORMAL, BEEP_ERROR_USR_PASWD);
					Access_LightStart(LIGHT_NG,LIGHT_SLOW,1,0);
					goto PROCEND;
				}
				return ;
			}
        }
		//App�㴦��
		//power_out_sleep();
		result = Proto_Packfun.AppProcCallBack(nettype,&pindat[PROTO_COMM_COMD_OFFSET],&Proto_PackOutbuf[PROTO_COMM_COMD_LENTHADD]);
		if(result == E_OK)
		{
			Proto_CommHostAckPack(nettype,&Proto_PackOutbuf[PROTO_COMM_COMD_LENTHADD]);
			goto PROCEND;
		}
	PROCEND: 
		Proto_Packfun.ProcEndCallBack(nettype,pindat[PROTO_COMM_COMD_CMDADD]);
}
/****************************************************************************************************
**Function:
	void Proto_CommSend(Proto_NetcommType nettype,uint8_t* pData) 
**Author: LXW
**Description:
**Input: 
**Output: 
****************************************************************************************************/
void Proto_CommSend(Proto_NetcommType nettype,uint8_t* pData) 
{
	uint8 databuf[8];
	uint8 checksum;
	uint8 lenth;
	if(pData[0] == 0)
	{
		return;
	}
	memcpy(&Proto_PackOutbuf[PROTO_COMM_COMD_LENTHADD],pData,PROTO_COMM_BRG_LENTH-2);
	
	Sys_GenFun16To8(PROTO_COMMPACK_HOST_DAT_OUT,Proto_PackOutbuf);
	Sys_GenFun16To8(Sys_PataCfg.SysId,(&Proto_PackOutbuf[4]));
	Sys_GenFun32To8(Sys_PataCfg.Mac,(&Proto_PackOutbuf[6]));
	//���ID ��
	Proto_PackOutbuf[PROTO_COMM_COMD_LENTHADD] += 1;//add checksum
	Proto_PackOutbuf[PROTO_COMM_COMD_LENTHADD] += 6;//add mac
#if(COMMENCPT_ENABLE == STD_TRUE)
	if((nettype == TCPIP_CLI)||(nettype == GPRS)||(nettype == WIFI))
	{
		if((Proto_PackOutbuf[PROTO_COMM_COMD_LENTHADD]&0x07) != 0)
		{
			Proto_PackOutbuf[PROTO_COMM_COMD_LENTHADD] = (Proto_PackOutbuf[PROTO_COMM_COMD_LENTHADD]&0xF8)+ 0x08;
		}
	}
#endif
	//����checksum 
	checksum = Sys_GenFunChecksum(&Proto_PackOutbuf[PROTO_COMM_COMD_LENTHADD]);
	Proto_PackOutbuf[Proto_PackOutbuf[PROTO_COMM_COMD_LENTHADD] + 2] = checksum;
	lenth = Proto_PackOutbuf[PROTO_COMM_COMD_LENTHADD]+3;
#if(COMMENCPT_ENABLE == STD_TRUE)	
	if(nettype == GPRS)
	{
		for(checksum= 0;checksum< ((lenth-3)>>3);checksum++)
		{
			memcpy(databuf,&Proto_PackOutbuf[3+(checksum<<3)],8);
			Encpt_EncryptExt(databuf);
			memcpy(&Proto_PackOutbuf[3+(checksum<<3)],databuf,8);
		}
	}
#endif
	Proto_NetCommSend(nettype,Proto_PackOutbuf,lenth);

}
/****************************************************************************************************
**Function:
	void Proto_CommHostPackRec(Proto_NetcommType nettype,uint8* pindat,uint16 lenth) 
**Author: LXW
**Description:
**Input: 
**Output: 
****************************************************************************************************/
static uint8 Proto_Packbuf[PROTO_COMM_BRG_BLE_LENTH];
static uint8 Proto_PackbufIndex=0;
static uint8 Proto_PackTempLenth =0;
extern void BleProc_timer_start(uint16 time);
void Proto_CommHostPackRec(Proto_NetcommType nettype,uint8* pindat,uint16 lenth) 
{
	if((pindat[0]== 0xaa)&&(pindat[1]== 0x55))
	{
		memset(Proto_Packbuf,0,sizeof(Proto_Packbuf));
		if(pindat[2] == 0xff)
		{
			Proto_PackTempLenth = 255;
			Proto_PackbufIndex = lenth;
		}
		else
		{
			Proto_PackTempLenth = pindat[2]+3;
			Proto_PackbufIndex = lenth;
		}
		memcpy(Proto_Packbuf,pindat,lenth);
		if(Proto_PackbufIndex >= Proto_PackTempLenth)
		{
			goto UPACK;
		}
	}
	else if(Proto_PackTempLenth == 255)
	{
		memcpy(&Proto_Packbuf[Proto_PackbufIndex],pindat,lenth);
		ProtoAnaly_ModifyCustomInfo(Proto_Packbuf);
		Proto_PackTempLenth = 0;
		Proto_PackbufIndex = 0;
	}
	else if(Proto_PackbufIndex < Proto_PackTempLenth)
	{
		memcpy(&Proto_Packbuf[Proto_PackbufIndex],pindat,lenth);
		Proto_PackbufIndex+= lenth; 
		if(Proto_PackbufIndex >= Proto_PackTempLenth)
		{
			goto UPACK;
		}
	}
		return;
UPACK:
	if(g_LockComuData.recIndex == 0)
	{
		g_LockComuData.recIndex = Proto_PackTempLenth;
		memcpy(g_LockComuData.pRecBuf,Proto_Packbuf,Proto_PackTempLenth);
		memset(Proto_Packbuf,0,sizeof(Proto_Packbuf));
		Proto_PackTempLenth = 0;
		Proto_PackbufIndex = 0;
		BleProc_timer_start(10);
	}
}

/****************************************************************************************************
**Function:
	void Proto_UpdateLearnRecord(uint8* pKeyId,uint32 time ,uint8 type, uint8 action)
**Author: rory
**Description:
**Input: 
**Output: 
****************************************************************************************************/
void  Proto_UpdateLearnRecord(uint8* pKeyId,uint32 time ,uint8 type, uint8 action)
{
	Proto_PackOutbuf[2] = 21;
	Proto_PackOutbuf[3] = COMD_LOAD_RECORD; 
	Proto_PackOutbuf[10] = E_OK;
	Proto_PackOutbuf[11] = 0;
	Proto_PackOutbuf[12] = 1;
	Proto_PackOutbuf[13] = 0;
	Proto_PackOutbuf[14] = 1;
	Proto_PackOutbuf[15] = type;
	if((type == ACCESS_LEARN_IDENTITY_CARD_TPYE) || (type == KEY_TYPE_IDENTITY_CARD))
	{
		Proto_PackOutbuf[16] = pKeyId[0];
		Proto_PackOutbuf[17] = pKeyId[1];
		Proto_PackOutbuf[18] = pKeyId[2];
		Proto_PackOutbuf[19] = pKeyId[3];
		Proto_PackOutbuf[20] = pKeyId[4];
		Proto_PackOutbuf[21] = pKeyId[5];
		Proto_PackOutbuf[22] = pKeyId[6];
		Proto_PackOutbuf[23] = pKeyId[7];
	}
	else
	{
		Proto_PackOutbuf[16] = pKeyId[0];
		Proto_PackOutbuf[17] = pKeyId[1];
		Proto_PackOutbuf[18] = pKeyId[2];
		Proto_PackOutbuf[19] = pKeyId[3];
		Proto_PackOutbuf[20] = 0xff;
		Proto_PackOutbuf[21] = 0xff;
		Proto_PackOutbuf[22] = 0xff;
		Proto_PackOutbuf[23] = 0xff;
	}
	Proto_PackOutbuf[24] = action;
	Proto_PackOutbuf[25] = TWOHI_UINT32(time);
	Proto_PackOutbuf[26] = TWOLO_UINT32(time);
	Proto_PackOutbuf[27] = ONEHI_UINT32(time);
	Proto_PackOutbuf[28] = ONELO_UINT32(time);
	Proto_CommHostAckPack(NRF_BLE,&Proto_PackOutbuf[PROTO_COMM_COMD_LENTHADD]);
	BleProc_timer_start(10);
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

