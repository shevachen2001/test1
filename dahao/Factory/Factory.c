#include "PUBLIC\Std_Types.h"
#include "Protocol\Proto_ComDef.h"
#include "FACTORY\Factory.h"
#include "ble_types.h"
#include "ble_dahao.h"
#include "MOTOR\motor.h"


uint8 Factory_State = 0;
uint8 Factory_Sub = 0;
uint8 Factory_Time64ms;

extern uint16_t m_conn_handle;

/****************************************************************************************************
**Function:
   void Factory_Entern_Sleep(void)
**Author: lxw
**Description:
**Input: 
**Output: 
****************************************************************************************************/
void Factory_Entern_Sleep(void)
{
	switch(Factory_Sub)
	{
		case CASE_IN:
		{
			if(m_conn_handle == BLE_CONN_HANDLE_INVALID)
			{
				Factory_Sub = CASE_01;
				Factory_Time64ms = 1000/64;
			}
			break;
		}
		case CASE_01:
		{
				Factory_Sub = CASE_END;
				ble_dahao_stop_advert();
				m_ble_adv_status = BLE_ADV_STOP;
				Factory_State = COMD_FATORY_IDLE;
			break;
		}
		default:
		{
			Factory_Sub = CASE_END;
			break;
		}
	}
}


/****************************************************************************************************
**Function:
   void Factory_Motor_Test(void)
**Author: lxw
**Description:
**Input: 
**Output: 
****************************************************************************************************/
void Factory_Motor_Test(void)
{
	switch(Factory_Sub)
	{
		case CASE_IN:
		{
			if(m_conn_handle == BLE_CONN_HANDLE_INVALID)
			{
				//MotorDrive.ioctl(MOTOR_CMD_CLOSE, NULL);
				Factory_Sub = CASE_01;
				Factory_Time64ms = 5000/64;
			}
			break;
		}
		case CASE_01:
		{ /* 关*/
			if((Motor_State == MOTOR_STAT_IDLE) || (Factory_Time64ms == 0))
			{
				Factory_Sub = CASE_02;
				Factory_Time64ms = 5000/64;
			}
			break;
		}
		case CASE_02:
		{/* 等待开锁*/
			if(Factory_Time64ms == 0)
			{
				//MotorDrive.ioctl(MOTOR_CMD_OPEN, NULL);
				Factory_Sub = CASE_03;
				Factory_Time64ms = 5000/64;
			}
			break;
		}
		case CASE_03:
		{ /* 开锁*/
			if((Motor_State == MOTOR_STAT_IDLE) || (Factory_Time64ms == 0))
			{
				Factory_Sub = CASE_04;
				Factory_Time64ms = 5000/64;
			}
			break;
		}
		case CASE_04:
		{ /* 等待关锁*/
			if (Factory_Time64ms == 0)
			{	/* 关锁*/
				//MotorDrive.ioctl(MOTOR_CMD_CLOSE, NULL);
				Factory_Sub = CASE_01;
				Factory_Time64ms = 5000/64;
			}
			break;
		}
		default:
		{
			Factory_Sub = CASE_END;
			break;
		}
	}
}

/****************************************************************************************************
**Function:
   void Factory_Proc(void)
**Author: lxw
**Description:
**Input: 
**Output: 
****************************************************************************************************/
void Factory_Proc(void)
{
	if(Factory_Time64ms != 0)
	{
		Factory_Time64ms--;
		return;
	}
	switch(Factory_State)
	{
		case COMD_FATORY_IDLE:
		{

			break;
		}
		case COMD_FATORY_ENTERN_SLEEP:
		{
			Factory_Entern_Sleep();
			break;
		}
		case COMD_FATORY_MOTOR_TEST:
		{
			Factory_Motor_Test();
			break;
		}
		default:
		{
			break;
		}
	}
}


