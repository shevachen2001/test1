/*****************************************************************************
|  File Name: DataUp.h
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
| 2016-05      Rory     Initial version 
|****************************************************************************/

#ifndef __LOCK_KEY_H
#define __LOCK_KEY_H

#include "PUBLIC\Std_Types.h"
#include "Access\LockKey.h"

/******************************************************************************
**********************Macro definition*******************************************
******************************************************************************/
#define KEY_READ_ONE_SIZE          10 /* ���ζ�ȡ��С*/

#define KEY_SIZE_MAX                    50  /* �ܵĴ洢����*/

#define USER_ID_ADDR                    0 /* user ID ��ʼ��ַ*/
#define ACTIVE_TIME_ADDR             (USER_ID_ADDR + KEY_SIZE_MAX*4) /* Active Time��ַ*/
#define APP_KEY_ADDR                 (ACTIVE_TIME_ADDR + KEY_SIZE_MAX*4) /* APP ��ʼ��ַ*/
#define APP_PASWD_ADDR               (APP_KEY_ADDR + KEY_SIZE_MAX*4) /* PASWD��ʼ��ַ*/
#define APP_CARD_ADDR                (APP_PASWD_ADDR + KEY_SIZE_MAX*4) /* CARD��ʼ��ַ*/
#define APP_FINGER_PRINT_ADDR        (APP_CARD_ADDR + KEY_SIZE_MAX*4) /* ָ����ʼ��ַ*/


#define BLACK_LIST_MAX               84//168//84
//#define BLACK_LIST_PAGEMAX         16

#define SERVE_KEY_MAX                200//120//60
#define SERVE_KEY_PAGEMAX            15

/******************************************************************************
**********************Type statement*******************************************
******************************************************************************/
typedef struct {
	uint16 BlackListCount; 
	uint16 ServKeyCount;
	uint16 AppKeyCount;
}LockKey_StatisticsType;	
	
typedef struct {
	uint8 Type; 
	uint32 KeyId;
	uint32 InvalidTime;
	
}LockKey_BlackType;	
typedef struct {
	LockKey_BlackType BlackList[BLACK_LIST_MAX];
	uint8 flashIndex;
	uint32 KeyWord;
}LockKey_BlackListType;	

typedef struct {
	uint8 Type; 
	uint8 Cycle;
	uint32 KeyId;
	uint32 StartTime;
	uint32 EndTime;
}LockKey_ServeKeyType;

typedef struct {
	uint32 KeyId1; 
	uint32 KeyId2;
	uint32 StartTime;
	uint32 EndTime;
}LockKey_IdentityKeyType; /* ���֤*/

typedef struct {
	LockKey_ServeKeyType KeyList[SERVE_KEY_MAX];
	uint32 KeyWord;
}LockKey_ServeKeyListType;

/******************************************************************************
**********************Variable definition*******************************************
******************************************************************************/
extern uint16 Access_CardLearnTimer64;

extern LockKey_StatisticsType LockKeyStatistics;
extern LockKey_BlackListType LockKeyBlackList;
extern LockKey_ServeKeyListType LockKeyServeKeyList;
extern uint8 Access_Record_Null;

/****************************************************************************/
/* Function                                                                 */
/****************************************************************************/
extern uint8 LockKey_Check_PaswdKey(uint8* pTouchKey,uint8 lenth);
extern uint8 LockKey_Check_CardKey(uint8 idtpye,uint8 *pid);
extern uint8 LockKey_Add_VisitCodeKey(uint8 idtype, uint32 keyId, uint32 startTime, uint32 endTime);
extern uint8 LockKey_Remove_VisitCodeKey(void);
extern uint8 LockKey_Start_LearnCardKey(uint8 *pData);
extern uint8 LockKey_Start_LearnCardKeyExt(uint8 *pData);

extern void LockKey_Default_Key(void);
extern uint8 LockKey_Read_Key(uint8 *pInData, uint8 *pOutData);
extern uint8 LockKey_Remove_Key(uint8 *pData, uint8 type);
extern uint8 LockKey_Remove_PasswordKey(void);

extern void BlackList_LoadPara(void);
extern void BlackList_StorePara(void);
extern void ServeKeyList_LoadPara(void);
extern void ServeKeyList_StorePara(void);
extern uint8 ServeKeyList_Remove(uint8 index);
extern void BlackList_Add(uint8 Type, uint32 KeyId, uint32 InvalidTime);
extern void BlackList_RemoveKey(uint8 idtpye, uint32 keyId);
extern uint8 BlackList_Remove(uint8 index);
extern void RecordList_LoadPara(void);
extern void RecordList_StorePara(uint8 mode);

extern void ServeKeyList_Readflash(uint8 offset);
extern void ServeKeyList_Writeflash(uint8 offset);
//extern void BlackList_Readflash(uint8 offset);
//extern void BlackList_Writeflash(uint8 offset);

#endif	
/************************ (C) COPYRIGHT DaHao electronics *****END OF FILE****/

