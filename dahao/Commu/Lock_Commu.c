
#include "COMMU\Lock_Commu.h"
#include "SYSTEM\Sys_GenFun.h"
#include "Encpt\Aes.h"
#include "Protocol\Proto_CommPack.h"
#include "protocol\Proto_NetComm.h"
#include "ble_dahao.h"
#include "SYSTEM\sys.h"
#include "Beep\Beep.h"

LockCommu_Type g_LockComuData;

//extern void Send_timer_start(Proto_NetcommType sttype,uint16 time);
/****************************************************************************************************
**Function:
	void LockCommu_Init(void)
**Author: rory
**Description:
**Input: 
**Output: 
****************************************************************************************************/
void LockCommu_Init(void)
{
	g_LockComuData.sendSt = TXD_MODE_IDLE;
//	g_LockComuData.recSt = RXD_MODE_IDLE;
//	g_LockComuData.recTime = 0;
//  g_LockComuData.sendTime = 0;
}

/****************************************************************************************************
**Function:
	void LockCommu_SendStart(uint8 *pData)
**Author: rory
**Description:
**Input: 
**Output: 
****************************************************************************************************/
void LockCommu_SendStart(uint8 *pData, uint8 Length)
{
	uint8 sendLength;

	memcpy(g_LockComuData.pSendBuf, pData, Length);
	g_LockComuData.sendLength = Length;
	g_LockComuData.sendSt = TXD_MODE_WAIT;
	if(g_LockComuData.sendLength <= BLE_FRAME_MAX_DATA_LEN)
	{
		sendLength = g_LockComuData.sendLength;
	}
	else
	{
		sendLength = BLE_FRAME_MAX_DATA_LEN;
	}
	ble_dahao_notify_data(&m_ble_dahao, g_LockComuData.pSendBuf, sendLength);
	g_LockComuData.sendIndex = sendLength;
	if(g_LockComuData.sendLength > g_LockComuData.sendIndex)
	{
	  //Send_timer_start(NRF_BLE,50);
	}
	else 
	{
		g_LockComuData.sendIndex = 0;
		g_LockComuData.sendLength = 0;
	}
}

uint32 MTime;
uint8 CheckTime(uint8_t *pInData)
{
    uint8 ret;
    MTime = BUILD_UINT32(pInData[3], pInData[2], pInData[1], pInData[0]);
	if(AccRcord.AppTimerBk < MTime )    
	{
		AccRcord.AppTimerBk = MTime;
		ret = E_OK;
	}
	else
	{
		ret = E_TIME_OUT;
	}
	return ret;
}


void LockCommu_Proc(void)
{
    uint8 ret;
	if(Sys_DisableBlueTooth())
	{
		return;
	}
	if(g_LockComuData.recIndex != 0)
	{
		uint8 aesdec[16];
		uint8 i;
		for(i= 0;i< (g_LockComuData.recIndex-3>>4);i++)
		{
			AES_Decrypt(&g_LockComuData.pRecBuf[3+(i<<4)],aesdec);
			memcpy(&g_LockComuData.pRecBuf[3+(i<<4)],aesdec,16);
		}

#if 0
		ret = CheckTime(&g_LockComuData.pRecBuf[g_LockComuData.recIndex - 5]);
		if ( E_OK != ret )  
		{
			return;
		}
#endif
		BLE_UPDATA_DISCONNECT_TIMER(7000/64);
		Proto_CommHostUnPackRec(NRF_BLE, &g_LockComuData.pRecBuf[0], g_LockComuData.recIndex);
		g_LockComuData.recIndex = 0;
		BleProc_timer_start(50);
		return ;
	}  
	
	
	if( Sys_DisableBlueTooth())
	{
		return ;
	}
	if(LockCommu_TimerProc() == E_NOT_OK)
	{
		BleProc_timer_start(50);
	}
}

Std_ReturnType LockCommu_TimerProc(void)
{
		uint8 Length, sendLength;
		Length = g_LockComuData.sendLength - g_LockComuData.sendIndex;
		if(Length > 0)
		{
			if(Length <= BLE_FRAME_MAX_DATA_LEN)
			{
				sendLength = Length;
				ble_dahao_notify_data(&m_ble_dahao, &g_LockComuData.pSendBuf[g_LockComuData.sendIndex], sendLength);

				g_LockComuData.sendLength= 0;
				g_LockComuData.sendIndex = 0;
				return E_OK;
			}
			else
			{
				sendLength = BLE_FRAME_MAX_DATA_LEN;
				ble_dahao_notify_data(&m_ble_dahao, &g_LockComuData.pSendBuf[g_LockComuData.sendIndex], sendLength);
				g_LockComuData.sendIndex += sendLength;
			}
			return E_NOT_OK;
		}
		else 
		{
			return E_OK;
		}
}
