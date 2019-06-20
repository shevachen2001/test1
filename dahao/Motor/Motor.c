#include "PUBLIC\Std_Types.h"
#include "MOTOR\motor.h"
#include "SYSTEM\sys.h"
#include "KEY\Touch.h"
#include "Protocol\Proto_ComDef.h"
#include "FACTORY\Factory.h"


Motor_StateType Motor_State=MOTOR_STAT_IDLE;
Motor_PositType Motor_PositStatus;

Motor_DoorLockType Motor_DoorLockSt;

uint8 Motor_SubState;
uint8 Motor_DetectSw=0xff;
uint16 Motor_Timer64ms;
uint8 Motor_AutoLockTimer64ms; /* 自动关锁*/
uint8 Motor_SwLR;  /* 左右检测*/
uint8 Motor_DoorSw;  /* 门磁检测开关*/

uint8 MotorLevelSw; /* 水平检测开关*/
uint8 MotorVerticalSw; /* 垂直检测开关*/
uint8 MotorBackSw; /* 回位检测开关*/

#define CheckLevelSwOn()      (MotorLevelSw > MOTOR_STATE_ON)
#define ChecVerticalSwOn()    (MotorVerticalSw > MOTOR_STATE_ON)
#define ChecBackSwOn()         (MotorBackSw < MOTOR_STATE_OFF)

#define MOTOR_WORK_TIME    5000/64

extern void Tsmxx_Irq_Init(void);

#if 0
/****************************************************************************************************
**Function:
   void Motor_Irq_Init(void)
**Author: lxw
**Description:
**Input: 
**Output: 
****************************************************************************************************/
void Motor_Irq_Init(void)
{
//	return;
#if 1
	nrf_gpio_cfg_sense_input(TOUCH_INTT_PIN, NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_SENSE_LOW);	 
    nrf_gpio_cfg_sense_input(DOOR_SW, NRF_GPIO_PIN_PULLUP, NRF_GPIO_PIN_SENSE_LOW);
	NRF_GPIOTE->INTENSET = 1<< 31;
	NVIC_SetPriority(GPIOTE_IRQn, 3);
	NVIC_ClearPendingIRQ(GPIOTE_IRQn);
	NVIC_EnableIRQ(GPIOTE_IRQn);
#else
	//  1、将相应的终端输入IO管脚设置为输入模式
	nrf_gpio_cfg_input(DOOR_SW,NRF_GPIO_PIN_PULLUP);//设置管脚位上拉输入
	
	//  2、通过CONFIG[n]设置GPIOTE通道
    	NRF_GPIOTE->CONFIG[1] =   (GPIOTE_CONFIG_POLARITY_HiToLo<<GPIOTE_CONFIG_POLARITY_Pos)  //触发方式POLARITY
                            | (DOOR_SW<<GPIOTE_CONFIG_PSEL_Pos)  //关联任务OUT[n]或者事件IN[n]的引脚PSEL
                            | (GPIOTE_CONFIG_MODE_Event<<GPIOTE_CONFIG_MODE_Pos); //模式

	//  3、通过INTENSET寄存器进行使能中断
	NRF_GPIOTE->INTENSET |= (GPIOTE_INTENSET_IN1_Set << GPIOTE_INTENSET_IN1_Pos);// 使能中断类型:
	NVIC_ClearPendingIRQ(GPIOTE_IRQn);
	NVIC_EnableIRQ(GPIOTE_IRQn);//使能GPIOTE中断
	NVIC_SetPriority(GPIOTE_IRQn,3);//设置中断优先级
#endif
}

/****************************************************************************************************
**Function:
   void Motor_Irq_Disable(void)
**Author: lxw
**Description:
**Input: 
**Output: 
****************************************************************************************************/
void Motor_Irq_Disable(void)
{
	//return;
	nrf_gpio_cfg_sense_input(DOOR_SW, NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_SENSE_LOW);	 
	NVIC_DisableIRQ(GPIOTE_IRQn);
}

/****************************************************************************************************
**Function:
   uint8 Motor_CheckLR(void)
**Author: rory
**Description:
**Input: 
**Output: 
****************************************************************************************************/
uint8 Motor_CheckLR(void)
{
	uint8 i;
	uint8 LR = 0;
	uint8 ret;
	
	nrf_gpio_cfg_input(SW_LR,NRF_GPIO_PIN_PULLUP);
	Sys_GenFunDelayUs(2);
	for(i=0; i<4; i++)
	{
		if(SW_LRGetData() == 0)
		{
			LR++;
		}
	}
	if(LR>2)
	{
		ret = 1;
	}
	else
	{
		ret = 0;
	}
	nrf_gpio_cfg_output(SW_LR);
	return ret;
}
/****************************************************************************************************
**Function:
	void Access_MotorOpen(void)
**Author: rory
**Description:
**Input: direction:1 顺时针  0:励时针
**Output: 
****************************************************************************************************/
void Access_MotorWorkOn(uint8 direction)
{
	//return;
	Vdd_PowerOn();
	nrf_gpio_cfg_output(LOCK_M_ADD_PIN);
	nrf_gpio_cfg_output(LOCK_M_MINUS_PIN);
	if(direction == 0)
	{
		nrf_gpio_pin_clear(LOCK_M_ADD_PIN); 
		nrf_gpio_pin_set(LOCK_M_MINUS_PIN);
	}
	else
	{
		nrf_gpio_pin_set(LOCK_M_ADD_PIN); 
		nrf_gpio_pin_clear(LOCK_M_MINUS_PIN);
	}
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
}
/****************************************************************************************************
**Function:
   int8 Motor_Open(void *pData)
**Author:rory
**Description:
**Input: 
**Output: 
****************************************************************************************************/
void Motor_Open(void *pData)
{
	nrf_gpio_cfg_output(COM_COL);
	nrf_gpio_cfg_input(COM_COL1,NRF_GPIO_PIN_PULLUP);
	nrf_gpio_cfg_input(COM_COL2,NRF_GPIO_PIN_PULLUP);
	nrf_gpio_cfg_input(SW_LR,NRF_GPIO_PIN_PULLUP);
	nrf_gpio_cfg_input(REVERSAL_SW,NRF_GPIO_PIN_PULLUP);
	nrf_gpio_cfg_input(DOOR_SW,NRF_GPIO_PIN_PULLUP);
	Motor_State = MOTOR_STAT_IDLE;
	Motor_DoorLockSt = MOTOR_DOORLOCK_IDLE;
//	Motor_Irq_Init();
	COM_COLHight();
	Motor_SwLR = Motor_CheckLR();
}
	

/****************************************************************************************************
**Function:
    Std_ReturnType Motor_Write(uint8 Cmd,void *pData)
**Author: rory
**Description:
**Input: 
**Output: 
****************************************************************************************************/
Std_ReturnType Motor_Write(void *pData)
{
	
	return E_OK;
}

/****************************************************************************************************
**Function:
    Std_ReturnType Motor_Write(uint8 Cmd,void *pData)
**Author: rory
**Description:
**Input: 
**Output: 
****************************************************************************************************/
Std_ReturnType Motor_Read(void *pData)
{
	if(Motor_State == MOTOR_STAT_IDLE)
	{
		return E_OK;
	}
	else
	{
		return E_NOT_OK;
	}
}

/****************************************************************************************************
**Function:
    Std_ReturnType Motor_Write(uint8 Cmd,void *pData)
**Author: rory
**Description:
**Input: 
**Output: 
****************************************************************************************************/
void Motor_DetectPosit(void)
{
	//nrf_gpio_cfg_input(COM_COL1,NRF_GPIO_PIN_PULLUP);
	//nrf_gpio_cfg_input(COM_COL2,NRF_GPIO_PIN_PULLUP);
	if(ComCol1GetData() == 0)
	{
		if(MotorLevelSw<MOTOR_STATE_MAX)
		{
			MotorLevelSw++;
		}
	}
	else
	{
		if(MotorLevelSw>MOTOR_STATE_MIN)
		{
			MotorLevelSw--;
		}
	}
	if(ComCol2GetData() == 0)
	{
		if(MotorVerticalSw<MOTOR_STATE_MAX)
		{
			MotorVerticalSw++;
		}
	}
	else
	{
		if(MotorVerticalSw>MOTOR_STATE_MIN)
		{
			MotorVerticalSw--;
		}
	}
	
	/*if(ReversalSwGetData() == 0)
	{
		if(MotorBackSw<MOTOR_STATE_MAX)
		{
			MotorBackSw++;
		}
	}
	else
	{
		if(MotorBackSw>MOTOR_STATE_MIN)
		{
			MotorBackSw--;
		}
	}*/

	if(Motor_SwLR == 1)
	{
		if(CheckLevelSwOn())
		{
			Motor_PositStatus = MOTOR_POSIT_UNLOCK;
			if(Sys_CheckEnableDoorLock())
			{
				if(Motor_DoorLockSt == MOTOR_DOORLOCK_IDLE)
				{
					Motor_DoorLockSt = MOTOR_DOORLOCK_OPENLOCK;
				}
			}
		}
		else if(ChecVerticalSwOn())
		{
			Motor_PositStatus = MOTOR_POSIT_LOCK;
		}
		else
		{
			Motor_PositStatus = MOTOR_POSIT_MIDDLE;
		}
	}
	else
	{
		if(CheckLevelSwOn())
		{
			Motor_PositStatus = MOTOR_POSIT_LOCK;
		}
		else if(ChecVerticalSwOn())
		{
			Motor_PositStatus = MOTOR_POSIT_UNLOCK;
		}
		else
		{
			Motor_PositStatus = MOTOR_POSIT_MIDDLE;
		}
	}
}

/****************************************************************************************************
**Function:
   void Motor_UnLockProc(uint8 LR)
**Author: rory
**Description:
**Input: LR = 1: Left   0:Right
**Output: 
****************************************************************************************************/
void Motor_UnLockProc(uint8 LR)
{
	switch(Motor_SubState)
	{
		case CASE_IN:
		{/* 判断方向*/
			if(LR == 1)
			{/* Left */
				if(CheckLevelSwOn())
				{
					//Access_LightStart(LIGHT_OK,LIGHT_SLOW,1,0);
					Access_BeepStart(BEEP_OPEN,1);
					Motor_SubState = CASE_OK;
					break;
				}
				else
				{
					Access_MotorWorkOn(1);
					Motor_SubState = CASE_01;
					Motor_Timer64ms = MOTOR_WORK_TIME;
				}
			}
			else
			{/* Right */
				if(ChecVerticalSwOn())
				{
					//Access_LightStart(LIGHT_OK,LIGHT_SLOW,1,0);
					Access_BeepStart(BEEP_OPEN,1);
					Motor_SubState = CASE_OK;
					break;
				}
				else
				{
					Access_MotorWorkOn(0);
					Motor_SubState = CASE_01;
					Motor_Timer64ms = MOTOR_WORK_TIME;
				}
			}
			Access_BeepStart(BEEP_NORMAL,1);
			break;
		}
		case CASE_01:
		{/* 2S 等待*/
			if(Motor_Timer64ms == 0)
			{
				//Access_MotorStop();
				Access_LightStart(LIGHT_NG,LIGHT_SLOW,1,0);
				Access_BeepStart(BEEP_FAST,3);
				Motor_SubState = CASE_EEROR;
				break;
			}
			if(LR == 1)
			{/* Left */
				if(CheckLevelSwOn())
				{
					Motor_SubState = CASE_02;
					Motor_Timer64ms = 1;
				}
			}
			else
			{/* Right */
				if(ChecVerticalSwOn())
				{
					Motor_SubState = CASE_02;
					Motor_Timer64ms = 1;
				}
			}
			break;
		}
		case CASE_02:
		{
			if(Motor_Timer64ms == 0)
			{
				//Access_MotorStop();
				Motor_Timer64ms = 1000/64;
				Motor_SubState = CASE_03;
				break;
			}
			break;
		}
		case CASE_03:
		{
			if(Motor_Timer64ms != 0)
			{
				break;
			}
			if(LR == 1)
			{/* Left */
				Access_MotorWorkOn(0);
			}
			else
			{/* Right */
				Access_MotorWorkOn(1);
			}
			Motor_Timer64ms = 500/64;
			Motor_SubState = CASE_04;
			break;
		}
		case CASE_04:
		{
			if(Motor_Timer64ms != 0)
			{
				break;
			}
			Motor_Timer64ms = 1500/64;
			Motor_SubState = CASE_05;
			nrf_gpio_cfg_input(REVERSAL_SW,NRF_GPIO_PIN_PULLUP);
			break;
		}
		case CASE_05:
		{
			if(Motor_Timer64ms == 0)
			{
				//Access_MotorStop();
				Access_LightStart(LIGHT_NG,LIGHT_SLOW,1,0);
				Access_BeepStart(BEEP_FAST,3);
				Motor_SubState = CASE_EEROR;
				break;
			}
			if(ChecBackSwOn())
			{
			#if 1
				if(LR == 1)
				{/* Left */
					Motor_Timer64ms = 500/64;
				}
				else
				{/* Right */
					Motor_Timer64ms = 0;
				}
			#else
				Motor_Timer64ms = 0;
			#endif
				Motor_SubState = CASE_06;
			}
			break;
		}
		case CASE_06:
		{
			if(Motor_Timer64ms == 0)
			{
				if(LR == 1)
				{/* Left */
					Access_MotorWorkOn(1);
				}
				else
				{/* Right */
					Access_MotorWorkOn(0);
				}
				Motor_SubState = CASE_07;
				Motor_Timer64ms = 1;
				break;
			}
			break;
		}
		case CASE_07:
		{
			if(Motor_Timer64ms == 0)
			{
				//Access_MotorStop();
				//Access_LightStart(LIGHT_OK,LIGHT_SLOW,1,0);
				Access_BeepStart(BEEP_OPEN,1);	
				Motor_SubState = CASE_END;
				break;
			}
			break;
		}
		case CASE_EEROR:
		case CASE_END:
		{
			break;
		}
	}
}


/****************************************************************************************************
**Function:
    Std_ReturnType Motor_Write(uint8 Cmd,void *pData)
**Author: rory
**Description:
**Input: LR = 1: Left   0:Right
**Output: 
****************************************************************************************************/
void Motor_LockProc(uint8 LR)
{
	switch(Motor_SubState)
	{
		case CASE_IN:
		{/* 判断方向*/
			if(LR == 1)
			{/* Left */
				if(ChecVerticalSwOn())
				{
					//Access_LightStart(LIGHT_OK,LIGHT_SLOW,1,0);
					Access_BeepStart(BEEP_OPEN,1);
					Motor_SubState = CASE_OK;
					break;
				}
				else
				{
					Access_MotorWorkOn(0);
					Motor_SubState = CASE_01;
					Motor_Timer64ms = MOTOR_WORK_TIME;
				}
			}
			else
			{/* Right */
				if(CheckLevelSwOn())
				{
					//Access_LightStart(LIGHT_OK,LIGHT_SLOW,1,0);
					Access_BeepStart(BEEP_OPEN,1);
					Motor_SubState = CASE_OK;
					break;
				}
				else
				{
					Access_MotorWorkOn(1);
					Motor_SubState = CASE_01;
					Motor_Timer64ms = MOTOR_WORK_TIME;
				}
			}
			Access_BeepStart(BEEP_NORMAL,1);
			break;
		}
		case CASE_01:
		{/* 2S 等待*/
			if(Motor_Timer64ms == 0)
			{
				//Access_MotorStop();
				Access_LightStart(LIGHT_NG,LIGHT_SLOW,1,0);
				Access_BeepStart(LIGHT_VERY_FAST,4);
				Motor_SubState = CASE_EEROR;
				break;
			}
			if(LR == 1)
			{/* Left */
				if(ChecVerticalSwOn())
				{
					Motor_SubState = CASE_02;
					Motor_Timer64ms = 1;
				}
			}
			else
			{/* Right */
				if(CheckLevelSwOn())
				{
					Motor_SubState = CASE_02;
					Motor_Timer64ms = 2;
				}
			}
			break;
		}
		case CASE_02:
		{
			if(Motor_Timer64ms == 0)
			{
				//Access_MotorStop();
				Motor_Timer64ms = 1000/64;
				Motor_SubState = CASE_03;
				break;
			}
			break;
		}
		case CASE_03:
		{
			if(Motor_Timer64ms != 0)
			{
				break;
			}
			if(LR == 1)
			{/* Left */
				Access_MotorWorkOn(1);
			}
			else
			{/* Right */
				Access_MotorWorkOn(0);
			}
			Motor_Timer64ms = 500/64;
			Motor_SubState = CASE_04;
			break;
		}
		case CASE_04:
		{
			if(Motor_Timer64ms != 0)
			{
				break;
			}
			Motor_Timer64ms = 1500/64;
			Motor_SubState = CASE_05;
			nrf_gpio_cfg_input(REVERSAL_SW,NRF_GPIO_PIN_PULLUP);
			break;
		}
		case CASE_05:
		{
			if(ReversalSwGetData() == 0)
			{
				if(MotorBackSw<MOTOR_STATE_MAX)
				{
					MotorBackSw++;
				}
			}
			else
			{
				if(MotorBackSw>MOTOR_STATE_MIN)
				{
					MotorBackSw--;
				}
			}
			if(Motor_Timer64ms == 0)
			{
				//Access_MotorStop();
				Access_LightStart(LIGHT_NG,LIGHT_SLOW,1,0);
				Access_BeepStart(BEEP_FAST,3);
				Motor_SubState = CASE_EEROR;
				break;
			}
			if(ChecBackSwOn())
			{
				Motor_SubState = CASE_06;
				if(LR == 1)
				{/* Left */
					Motor_Timer64ms = 0;//1000/32;
				}
				else
				{
					Motor_Timer64ms = 0;
				}
			}
			break;
		}
		case CASE_06:
		{
			if(Motor_Timer64ms == 0)
			{	
				if(LR == 1)
				{/* Left */
					Access_MotorWorkOn(0);
				}
				else
				{/* Right */
					Access_MotorWorkOn(1);
				}
				Motor_SubState = CASE_07;
				Motor_Timer64ms = 1;
				break;
			}
			break;
		}
		case CASE_07:
		{
			if((Motor_Timer64ms == 0) /*|| (ChecBackSwOn() == 0)*/)
			{
				//Access_MotorStop();
				//Access_LightStart(LIGHT_OK,LIGHT_SLOW,1,0);
				Access_BeepStart(BEEP_OPEN,1);	
				Motor_SubState = CASE_END;
				break;
			}
			break;
		}
		case CASE_EEROR:
		case CASE_END:
		{
			break;
		}
	}
}


/****************************************************************************************************
**Function:
    Std_ReturnType Motor_Write(uint8 Cmd,void *pData)
**Author: rory
**Description:
**Input: 
**Output: 
****************************************************************************************************/
void Motor_Proc(void)
{
	if((Motor_DetectSw>0)&&(Motor_DetectSw<4))
	{
		//COM_COLHight();
		Motor_DetectPosit();
	}
	
	switch(Motor_State)
	{
		case MOTOR_STAT_IDLE:
		{
			break;
		}
		case MOTOR_STAT_LOCK:
		{
			Motor_LockProc(Motor_SwLR);
			if(Motor_SubState == CASE_OK)
			{
				Motor_State = MOTOR_STAT_IDLE;
				break;
			}
			else if((Motor_SubState == CASE_END)||(Motor_SubState == CASE_EEROR))
			{
				Motor_State = MOTOR_STAT_IDLE;
				TouchRstTimes = 1;
				ble_dahao_start_advert(1);
				break;
			}
			break;
		}
		case MOTOR_STAT_UNLOCK:
		{
			Motor_UnLockProc(Motor_SwLR);
			if(Motor_SubState == CASE_OK)
			{
				Motor_State = MOTOR_STAT_IDLE;
				if(Sys_CheckAutoLock())
				{
					Motor_AutoLockTimer64ms = Sys_PataCfg.OpenLockTime/64;
				}
				break;
			}
			else if(Motor_SubState == CASE_EEROR)
			{
				Motor_State = MOTOR_STAT_IDLE;
				TouchRstTimes = 1;
				break;
			}
			else if(Motor_SubState == CASE_END)
			{
				ble_dahao_start_advert(1);
				if(Factory_State != COMD_FATORY_MOTOR_TEST)
				{
					if(Sys_CheckAutoLock())
					{
						Motor_AutoLockTimer64ms = Sys_PataCfg.OpenLockTime/64;
					}
					if(Sys_CheckEnableDoorLock())
					{
						Motor_Timer64ms = 5000/64;
						Motor_State = MOTOR_STAT_WAITOPENDOOR;	
					}
					else
					{
						Motor_State = MOTOR_STAT_IDLE;
					}
					TouchRstTimes = 1;
				}
				else
				{
					Motor_State = MOTOR_STAT_IDLE;
				}
			}
			break;
		}
		case MOTOR_STAT_WAITOPENDOOR:
		{
			if(DoorIsOpen())
			{
				Motor_DoorLockSt = MOTOR_DOORLOCK_OPEN;
				Motor_State = MOTOR_STAT_IDLE;	
				break;
			}
			else if(Motor_Timer64ms == 0)
			{
				Motor_State = MOTOR_STAT_IDLE;
				Motor_DoorLockSt = MOTOR_DOORLOCK_OPENLOCK;
				break;
			}
			break;
		}
		default:
		{	
			Motor_State = MOTOR_STAT_IDLE;
		}
	}
}
/****************************************************************************************************
**Function:
    Std_ReturnType Motor_IoCtl(uint8 Cmd,void *pData)
**Author: rory
**Description:
**Input: 
**Output: 
****************************************************************************************************/
 Std_ReturnType Motor_IoCtl(uint8 Cmd,void *pData)
 {
 	switch(Cmd)
 	{
		case MOTOR_CMD_SCAN:
		{
			if(Motor_Timer64ms != 0)
			{
				Motor_Timer64ms--;
			}
			if(Motor_AutoLockTimer64ms != 0)
			{
				Motor_AutoLockTimer64ms--;
				if(Motor_AutoLockTimer64ms == 0)
				{
					Access_Lock();
				}
			}
			if(Motor_DetectSw == 0xff)
			{
				COM_COLHight();
				nrf_gpio_cfg_input(COM_COL1,NRF_GPIO_PIN_PULLUP);
				nrf_gpio_cfg_input(COM_COL2,NRF_GPIO_PIN_PULLUP);
			}
			else if(Motor_State !=MOTOR_STAT_IDLE)
			{
				COM_COLHight();
				nrf_gpio_cfg_input(COM_COL1,NRF_GPIO_PIN_PULLUP);
				nrf_gpio_cfg_input(COM_COL2,NRF_GPIO_PIN_PULLUP);
				Motor_DetectSw = 2;
				break;
			}
			else if(Motor_DetectSw == 4)
			{
				COM_COLLow();
			}
			else if(Motor_DetectSw > 8)
			{
				Motor_DetectSw = 0xfe;
			}
			Motor_DetectSw++;
			break;
		}
		case MOTOR_CMD_OPEN:
		{
			if(Motor_State == MOTOR_STAT_IDLE)
			{
				Motor_State = MOTOR_STAT_UNLOCK;	
				Motor_SubState = CASE_IN;	
				Motor_SwLR = Motor_CheckLR();
			}
			break;
		}
		case MOTOR_CMD_CLOSE:
		{
			if((Motor_State == MOTOR_STAT_IDLE) || (Motor_State == MOTOR_STAT_WAITOPENDOOR))
			{
				Motor_State = MOTOR_STAT_LOCK;	
				Motor_DoorLockSt = MOTOR_DOORLOCK_IDLE;
				Motor_SubState = CASE_IN;
				Motor_SwLR = Motor_CheckLR();
			}
			break;
		}
		case MOTOR_CMD_SCAN_DOOR:
		{
			nrf_gpio_cfg_input(DOOR_SW,NRF_GPIO_PIN_PULLUP);
			Sys_GenFunDelayUs(2);
			if(DoorSwGetData() == 0)
			{
				if(Motor_DoorSw < 3)
				{
					Motor_DoorSw++;
					if(Motor_DoorSw == 3)
					{
						if((Motor_DoorLockSt == MOTOR_DOORLOCK_OPEN) && (Sys_CheckEnableDoorLock())
							&& (Motor_PositStatus != MOTOR_POSIT_LOCK))
						{
							Access_Lock();
						}
					}
				}
			}
			else
			{
				if(Motor_DoorSw > 0)
				{
					Motor_DoorSw--;
					if(Motor_DoorSw == 0)
					{
						if(Motor_DoorLockSt == MOTOR_DOORLOCK_OPENLOCK)
						{/* 门被打开*/
							Motor_DoorLockSt = MOTOR_DOORLOCK_OPEN;
						}
					}
				}
			}
			nrf_gpio_cfg_output(DOOR_SW);
			break;
		}
		default:break;
	}
	return E_OK;
 }

/****************************************************************************************************
**Function:
   Std_ReturnType Motor_Close(void *pData)
**Author: lxw
**Description:
**Input: 
**Output: 
****************************************************************************************************/
void Motor_Close(void *pData)
{
	nrf_gpio_cfg_output(COM_COL);
	nrf_gpio_cfg_output(COM_COL1);
	nrf_gpio_cfg_output(COM_COL2);
	nrf_gpio_cfg_output(REVERSAL_SW);
	COM_COLLow();
	COM_COL1Low();
	COM_COL2Low();
	ReversalSwLow();
	//Motor_State = MOTOR_STAT_IDLE;
}

const Dr_OpertonType MotorDrive =
{
	Motor_Open, Motor_Close, Motor_Read, Motor_Write, Motor_IoCtl
};

#if (MOTOR_CHECK == STD_TRUE)
void Motor_Check(void)
{
	MotorDrive.open(NULL);
	while(1)
	{
		//Motor_DetectPosit();
	}
}
#endif
#endif

