/*****************************************************************************
|  File Name: sys.h
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

#ifndef SYS_H_
#define SYS_H_

#include "PUBLIC\Std_Types.h"
#include "lora\radio.h"
#include "Access\Access.h"

/******************************************************************************
**********************Macro definitiont*******************************************
******************************************************************************/
#define BATT_VOLPER_MAX	4

#define				SYSID						0xffff
#define				CARLOCKID					10

#define             PUBLIC_BUID_MAX                   64  
#define             BUILD_MAX                               10
#define 		   UPDATE_CHECKSUM				0x55667788


#define				SYSBIGFLG						0x01
#define				SYSCARD3BYTE					0x02
#define				DETCARDSLEEP					0x04   /* ��Ƭ���߼��*/
#define				AUTOLOCK					       0x08   /* �Զ�����*/
#define				BTSAVEMODE				       0x10  /* ���������㲥*/
#define                        DISIBLEDOORLOCK                          0X20  /* �Ŵż��*/
#define                        DISIBLEBLUETOOTH                        0X40


#define WKUP_BLE  			   	0                                         
#define WKUP_KEY_INT               	1                                         
#define WKUP_LORA_INT            	2                                         
#define WKUP_LORA_TIME            	3                                         
#define 			SYS_MAXID					48
#define 			SYS_MINID					20

#define Sys_CheckCardSleepDet()      (Sys_PataCfg.CfgFlag&DETCARDSLEEP)
#define Sys_CheckBTSleepMode()      (Sys_PataCfg.CfgFlag&BTSAVEMODE)
#define Sys_CheckAutoLock()             (Sys_PataCfg.CfgFlag&AUTOLOCK)
#define Sys_CheckDisableAutoLock() ((Sys_PataCfg.CfgFlag&AUTOLOCK) == 0)
#define Sys_CheckDisableDoorLock() (Sys_PataCfg.CfgFlag&DISIBLEDOORLOCK)
#define Sys_CheckEnableDoorLock()  ((Sys_PataCfg.CfgFlag&DISIBLEDOORLOCK) == 0)
#define Sys_EnableBlueTooth()      ((Sys_PataCfg.CfgFlag&(uint8)DISIBLEBLUETOOTH) == 0)
#define Sys_DisableBlueTooth()      ((Sys_PataCfg.CfgFlag&(uint8)DISIBLEBLUETOOTH))
/******************************************************************************
**********************Type statement*******************************************
******************************************************************************/
typedef enum 
{
	SYS_STARTUP 		= 0,
	SYS_NORMAL,
	SYS_SLEEP,
}Sys_StateType ;

typedef struct {
	uint16 	SerSocketPort;		// Զ�̶˿�
	uint32  remoteCommIp;		//������  IP
	char 	TargetDnsName[40];	// ����
	char 	ApnCh[20];
}TcpIpPataType;//����ģʽ

typedef struct {
	uint32	remoteCommIp;		//������  IP
	char 	TargetDnsName[40];
	uint32	CheckSum;
}FtpInfoType;

typedef struct {
	uint16	connectOff;		//�Ͽ�����
	uint8       txPower;             // ���书��
	uint16     advInt;                // �㲥���
}BLEInfoType;

typedef struct {
	Radio_ParaType  rfPara;
	BLEInfoType  bleCfg;
	uint16   CustomId; /* ���ڷ�������������*/
	uint8 	AppSleepBread;
	uint8 	Headadd;
	uint8 	SyncIndex;
	uint8   usedpage;
	uint8 	EncpyPsd[8];
	uint8 	AarmEnable; /* ������ʾ1:����;  0:�ر�*/
	uint16  randindex;
	uint16 	SysId;
	uint8 	Aeskey[16];
	uint32	HeadMac;
	uint32 	Pswd;
	uint32 	Mac;
	uint16 	OpenLockTime; /* ��������ʱ��*/
	uint16     MotorTime; /* �������ʱ��*/
	uint16 	HeartTime; /* ����֡ʱ��* 16ms */
	uint8 	BuildId; /* ��ID  1 ~ 255 */    // 0 :������
	uint16 	PropertyId; /*�Ƶ�ID :0 ~ 65535 */
	uint16  HotelPswd;
	uint8 	CfgFlag; /*������Ϣ*/
	uint32  StartTime; // ��̬���뿨Ƭ��ʼʱ��
	uint8   touchSensitive; // �����������ȵ���
	uint8   UploadRecord;
	uint8   State;
	uint32  KeyWord;
	uint8   LongKeyFlag;
}Sys_PataCfgType;

/******************************************************************************
**********************Variable definition******************************************
******************************************************************************/
extern Sys_PataCfgType Sys_PataCfg;
extern uint8 Sys_CheckUpFlag ;
extern uint8  Sys_McuRestFlag;
extern uint8  Sys_StoreFlag;
extern uint16 Sys_RstTimsoutx64ms;
extern uint16 Sys_StoreTimsoutx64ms;
extern uint32 Sys_Tick ;
extern uint8  Sys_WkupType;
extern uint8  Sys_ResetDevice;
extern Sys_StateType Sys_State;
extern uint8  Sys_frstPoweron;
extern const uint16 Batt_VolPerTable[BATT_VOLPER_MAX][2];
extern uint16 Sys_SleepErrTimsoutx64ms ;

/******************************************************************************
**********************Function declared*******************************************
******************************************************************************/
extern void Sys_EnterWaitWakeUp(void);
extern void Sys_IoEnterSleep(void);
extern void Sys_IoExitSleep(void);
extern void Sys_IoStartUp(void);
extern void Sys_InitClock(void);
extern void Sys_StorePara(void);
extern void Sys_LoadPara(void);
extern void Sys_Init(void);
extern void Sys_StateProc(void);
extern void Sys_Parainit(void);
extern void Sys_ParainitFirst(void);
extern void Sys_RestProc(void);
extern uint8 Sys_GetBat(void);
extern void Sys_wdt_feed(void);  
extern void Sys_wdt_start(void);  
#if (defined DEBUG_ENABLE) && (DEBUG_ENABLE == STD_TRUE)
extern void  Sys_Debug(uint8 mode);
#endif

extern void Sys_SpiOpen(void);
extern void Sys_SpiClose(void);

#endif
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

