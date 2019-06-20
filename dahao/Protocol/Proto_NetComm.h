/*******************************************************************************************************************
|  File Name: Sys_NetComm.h
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
| 2013-10-23  1.00                  Initial version 
|******************************************************************************************************************/

#ifndef SYS_NETCOMM_H
#define SYS_NETCOMM_H
#include"PUBLIC\Std_Types.h"

/******************************************************************************
**********************Macro definitiont*******************************************
******************************************************************************/
typedef enum
{	
	NET_NULL = 0,
	NET_UART1,
	NET_UART2,
	NET_UART3,
	NET_UART4,
	NET_UART5,
	NET_UART6,
	ZGBEE,
	CC2540,
	NRF_BLE=CC2540,
	RS485_1,
	GPRS,
	WIFI,
	LORA,
	TCPIP_SER,
	TCPIP_CLI,
	TCPIP_BCT,
	READ_HEARD,
}Proto_NetcommType;





/******************************************************************************
**********************Type statement*******************************************
******************************************************************************/


/******************************************************************************
**********************Variable definition***************************************
******************************************************************************/

/******************************************************************************
**********************Function declared****************************************
******************************************************************************/
extern void Proto_NetCommSend(Proto_NetcommType CommType,uint8_t* pData, uint16 Len); 
extern uint16 Proto_NetCommRec(Proto_NetcommType CommType,uint8_t* pData); 
#endif

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
