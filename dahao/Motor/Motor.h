/*****************************************************************************
|  File Name: Motor.h
|
|  Description: Header File defined
|
|-----------------------------------------------------------------------------
|               C O P Y R I G H T
|-----------------------------------------------------------------------------
|-----------------------------------------------------------------------------
|               A U T H O R   I D E N T I T Y
|-----------------------------------------------------------------------------
| Initials     Name                      Company
| --------     ---------------------     -------------------------------------
| LXW          LXW               
|-----------------------------------------------------------------------------
|               R E V I S I O N   H I S T O R Y
|-----------------------------------------------------------------------------
| Date        Ver     Author  Description
| ---------   ----    ------  ------------------------------------------------
| 2013-10      LXW     Initial version 
|****************************************************************************/

#ifndef MOTOR_H
#define MOTOR_H
#include "PUBLIC\Std_Types.h"

/******************************************************************************
**********************Macro definition*******************************************
******************************************************************************/
#define MOTOR_CHECK		STD_FALSE


/******************************************************************************
**********************Type statement*******************************************
******************************************************************************/	
typedef enum
{
	MOTOR_CMD_SCAN = 0,
	MOTOR_CMD_OPEN,
	MOTOR_CMD_CLOSE,
	MOTOR_CMD_AUTO_CLOSE,/* 门磁检测关门*/
	MOTOR_CMD_SCAN_DOOR, /* 扫描门磁*/
	MOTOR_CMD_MAX
}Motor_CmdType;

typedef enum
{
	MOTOR_STAT_IDLE = 0,
	MOTOR_STAT_LOCK,
	MOTOR_STAT_UNLOCK,
       MOTOR_STAT_WAITOPENDOOR, /* 等待门打开*/
}Motor_StateType;

typedef enum
{
	MOTOR_DOORLOCK_IDLE = 0,
	MOTOR_DOORLOCK_OPENLOCK,
	MOTOR_DOORLOCK_OPEN,
}Motor_DoorLockType;

typedef enum
{
	MOTOR_POSIT_LOCK,
	MOTOR_POSIT_MIDDLE,
	MOTOR_POSIT_UNLOCK,
}Motor_PositType;

#define MOTOR_STATE_ON          2
#define MOTOR_STATE_OFF         2
#define MOTOR_STATE_MAX         4
#define MOTOR_STATE_MIN         0

/******************************************************************************
**********************Variable definition*******************************************
******************************************************************************/
extern Motor_StateType Motor_State;
extern uint8 Motor_DoorSw;
extern Motor_PositType Motor_PositStatus;
extern Motor_DoorLockType Motor_DoorLockSt;
extern uint8 Motor_AutoLockTimer64ms;

extern const Dr_OpertonType MotorDrive;

#define DoorIsClose()    (Motor_DoorSw > 2/*MOTOR_STATE_ON*/)
#define DoorIsOpen()    (Motor_DoorSw < 1/*MOTOR_STATE_ON*/)
/******************************************************************************
**********************Function declared*******************************************
******************************************************************************/
extern void Motor_Irq_Init(void);
extern void Motor_Proc(void);
extern void Access_MotorStop(void);
#if (MOTOR_CHECK == STD_TRUE)
extern void Motor_Check(void);
#endif
#endif	
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/


