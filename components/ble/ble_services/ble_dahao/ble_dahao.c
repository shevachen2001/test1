/*
*	
*
**/

#include "ble_dahao.h"
#include "ble_conn_params.h"
//#include "ble_stack_handler.h"
#include "ble_types.h"
#include "app_uart.h"
#include "Access\Access.h"
#include "SYSTEM\sys.h"
#include "CustomCode\CustomCode.h"
#include "Commu\Lock_Commu.h"
#include "protocol\Proto_NetComm.h"
#include "Protocol\Proto_CommPack.h"
#include "ble_advertising.h"
#include "KEY\Touch.h"
#include "MOTOR\motor.h"
#include "Access\LockKey.h"

extern void dahao_data_handler(uint8_t * p_data, uint16_t length);

uint8_t m_addl_adv_manuf_data[BLE_MANUF_DATA_LEN];
uint8_t m_addl_adv_data [BLE_FRAME_MAX_DATA_LEN];
uint8_t m_mac_add [6];
uint16_t m_disconnect_timer = 0;
uint8_t m_ble_adv_timer = 0;
uint8_t m_ble_adv_status = 0;
uint8_t m_ble_adv_restart_timer = 0;

/*****************************************************************************
* data handle
*****************************************************************************/
 data_info g_send_data = {NULL, 0, 0};
 data_info g_rcv_data = {NULL, 0, 0};

/**@brief     Function for handling the @ref BLE_GATTS_EVT_WRITE event from the S110 SoftDevice.
 *
 * @param[in] p_ble_evt Pointer to the event received from BLE stack.
 */
static void on_write(ble_dahao_t *p_wcs, ble_evt_t * p_ble_evt)
{
		//int error_code;
		ble_gatts_evt_write_t * p_evt_write = &p_ble_evt->evt.gatts_evt.params.write;
		if (p_evt_write->handle == p_wcs->read_write_handles.value_handle &&
				p_evt_write->len <= BLE_FRAME_MAX_DATA_LEN) 
		{
			Proto_CommHostPackRec(NRF_BLE,p_evt_write->data, p_evt_write->len);
		}
}


void ble_dahao_on_ble_evt(ble_dahao_t *p_wcs,ble_evt_t * p_ble_evt) {
    switch (p_ble_evt->header.evt_id)
    {
	case BLE_GAP_EVT_CONNECTED:
	{
		p_wcs->conn_handle = p_ble_evt->evt.gap_evt.conn_handle;	
		break;
	}
       case BLE_GATTS_EVT_WRITE:
	{
            on_write(p_wcs,p_ble_evt);
            break;
        }
        case BLE_GATTS_EVT_HVC:
	{
		//on_indicate_comfirm(p_wcs,p_ble_evt,p_data_handler);
		break;	
    	}
	case BLE_GAP_EVT_DISCONNECTED:
	{
		p_wcs->conn_handle = BLE_CONN_HANDLE_INVALID;
		break;
	}
        default:
            break;
    }
}

//Add WeChat Service to GATTS sercvice
uint32_t ble_dahao_add_service(ble_dahao_t *p_dahaoBle)
{
		uint32_t err_code;
		ble_uuid_t ble_dahao_uuid;
		BLE_UUID_BLE_ASSIGN(ble_dahao_uuid, BLE_UUID_DAHAO_SERVICE);
		err_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY, &ble_dahao_uuid, &p_dahaoBle->service_handle);
		return err_code;
}

void get_mac_addr(uint8_t *p_mac_addr)
{
#if 0
		uint32_t error_code;
	
		ble_gap_addr_t *p_mac_addr_t = (ble_gap_addr_t*)malloc(sizeof(ble_gap_addr_t));
		error_code = sd_ble_gap_address_get(p_mac_addr_t);
		APP_ERROR_CHECK(error_code);
		uint8_t *d = p_mac_addr_t->addr;
		for ( uint8_t i = 6; i >0;)
		{	
			i--;
			p_mac_addr[5-i]= d[i];
		}
		free(p_mac_addr_t);
		p_mac_addr_t = NULL;
#else
	uint32_t error_code;
	
		ble_gap_addr_t p_mac_addr_t;
		error_code = sd_ble_gap_address_get(&p_mac_addr_t);
		APP_ERROR_CHECK(error_code);
		uint8_t *d = p_mac_addr_t.addr;
		for ( uint8_t i = 6; i >0;)
		{	
			i--;
			p_mac_addr[5-i]= d[i];
		}
#endif
}

//Add the write characteristic 
static uint32_t ble_dahao_add_read_write_char(ble_dahao_t *p_dahaoBle)
{
    ble_gatts_char_md_t char_md;
    ble_gatts_attr_t    attr_char_value;
    ble_uuid_t          char_uuid;
    ble_gatts_attr_md_t attr_md;
    char *data = "read write char";
	
    memset(&char_md, 0, sizeof(char_md));
    char_md.char_props.write = 1;
    char_md.char_props.notify = 1;
    char_md.char_props.write_wo_resp = 1;
    char_md.p_char_user_desc  = NULL;
    char_md.p_char_pf         = NULL;
    char_md.p_user_desc_md    = NULL;
    char_md.p_cccd_md         = NULL;
    char_md.p_sccd_md         = NULL;
	
    BLE_UUID_BLE_ASSIGN(char_uuid, BLE_UUID_DAHAO_READ_WRITE_CHARACTERISTICS);
	
    memset(&attr_md, 0, sizeof(attr_md));
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.write_perm);
    attr_md.vloc       = BLE_GATTS_VLOC_STACK;
    attr_md.rd_auth    = 0;
    attr_md.wr_auth    = 0;
    attr_md.vlen       = 1;
    
    memset(&attr_char_value, 0, sizeof(attr_char_value));
    
    attr_char_value.p_uuid       = &char_uuid;
    attr_char_value.p_attr_md    = &attr_md;
    attr_char_value.init_len     = strlen(data);
    attr_char_value.init_offs    = 0;
    attr_char_value.max_len      = BLE_FRAME_MAX_DATA_LEN;
    attr_char_value.p_value      = (uint8_t *)data;
    
    return sd_ble_gatts_characteristic_add(p_dahaoBle->service_handle,
                                           &char_md,
                                           &attr_char_value,
                                           &p_dahaoBle->read_write_handles);
}


//Add the WeChat characteristic include indicate write and read characteristic
uint32_t ble_dahao_add_characteristics(ble_dahao_t *p_dahaoBle)
{
		uint32_t err_code;
	
		err_code = ble_dahao_add_read_write_char(p_dahaoBle);
		APP_ERROR_CHECK(err_code);
		return err_code;
}


uint32_t ble_dahao_notify_data(ble_dahao_t *p_wcs, uint8_t *data, uint16_t len)
{
		ble_gatts_hvx_params_t hvx_params;
		
		if (data == NULL || len == 0) {
				return 0;	
		}
	
		memset(&hvx_params, 0, sizeof(hvx_params));
		hvx_params.handle = p_wcs->read_write_handles.value_handle;
    		hvx_params.p_data = data;
    		hvx_params.p_len  = &len;
    		hvx_params.type   = BLE_GATT_HVX_NOTIFICATION;
		return sd_ble_gatts_hvx(p_wcs->conn_handle, &hvx_params);
}

void ble_dahao_get_scan_data(void)
{     
	uint8_t     i, j;
	uint8_t     hex[] = "0123456789ABCDEF";
	uint8_t     temp;
	//uint8 macAddr[6];

	memset(m_addl_adv_data, '\0', sizeof(m_addl_adv_data));
#if ((defined LOCK_TYPE) && (LOCK_TYPE == LOCK_FLAT))
	m_addl_adv_data[0] = hex[TYPE_LOCK_FLAT >> 4];
	m_addl_adv_data[1] = hex[TYPE_LOCK_FLAT & 0x0F];
#elif ((defined LOCK_TYPE) && (LOCK_TYPE == LOCK_HOTEL_FLAT))
	m_addl_adv_data[0] = hex[TYPE_LOCK_HOTEL >> 4];    //
	m_addl_adv_data[1] = hex[TYPE_LOCK_HOTEL & 0x0F];  // TYPE_LOCK_HOTEL
#elif ((defined LOCK_TYPE) && (LOCK_TYPE == LOCK_HOTEL_FLAT_REMOTE))
	m_addl_adv_data[0] = hex[TYPE_LOCK_HOTEL_REMOTE >> 4];
	m_addl_adv_data[1] = hex[TYPE_LOCK_HOTEL_REMOTE & 0x0F];
#endif

	get_mac_addr(m_mac_add);
	//Sys_PataCfg.Mac = BUILD_UINT32(macAddr[5], macAddr[4], macAddr[3], macAddr[2]);
	for ( i=0,j=2; i < 4; i++ )
	{
		temp = m_mac_add[2+i];
		m_addl_adv_data[j++] = hex[temp >> 4];
		m_addl_adv_data[j++] = hex[temp & 0x0F];
	}
#if (LOCK_TYPE  == LOCK_FLAT)
	m_addl_adv_data[10] = 'F';
#elif ((LOCK_TYPE  == LOCK_HOTEL_FLAT) || (LOCK_TYPE  == LOCK_HOTEL_FLAT_REMOTE))
	m_addl_adv_data[10] = 'H';
#endif
	m_addl_adv_data[11] = '_';
	for ( i=0,j=12; i < 4; i++ )
	{
		temp = m_mac_add[2+i];
		m_addl_adv_data[j++] = hex[temp >> 4];
		m_addl_adv_data[j++] = hex[temp & 0x0F];
	}
}

#define TOUCH_LIGHT_ON  0x01
#define INITIAL_STATUS    0x02
#define LOCK_STATUS         0x04
#define RECORD_STATUS     0x08


#define SW_VERSION    2

#if ((TOUCH_ENABLE == STD_TRUE) && (LORA_ENABLE == STD_TRUE) && (RC523_TYPEB == STD_TRUE) )
#define HW_VERSION    3  
#elif ((TOUCH_ENABLE == STD_TRUE) && (LORA_ENABLE == STD_TRUE) && (RC523_TYPEB == STD_FALSE) )
#define HW_VERSION    2
#elif ((TOUCH_ENABLE == STD_TRUE) && (LORA_ENABLE == STD_FALSE) && (RC523_TYPEB == STD_FALSE) )
#define HW_VERSION    1
#elif ((TOUCH_ENABLE == STD_FALSE) && (LORA_ENABLE == STD_TRUE) && (RC523_TYPEB == STD_TRUE) )
#define HW_VERSION    6
#elif ((TOUCH_ENABLE == STD_FALSE) && (LORA_ENABLE == STD_TRUE) && (RC523_TYPEB == STD_FALSE) )
#define HW_VERSION    5
#elif ((TOUCH_ENABLE == STD_FALSE) && (LORA_ENABLE == STD_FALSE) && (RC523_TYPEB == STD_FALSE) )
#define HW_VERSION    4
#endif

void ble_dahao_get_manual_data(void)
{
#if ((defined LOCK_TYPE) && (LOCK_TYPE == LOCK_FLAT))
	m_addl_adv_manuf_data[0] = TYPE_LOCK_FLAT<<2;
#elif ((defined LOCK_TYPE) && (LOCK_TYPE == LOCK_HOTEL_FLAT))
	m_addl_adv_manuf_data[0] = TYPE_LOCK_HOTEL<<2;        //  
#elif ((defined LOCK_TYPE) && (LOCK_TYPE == LOCK_HOTEL_FLAT_REMOTE))
	m_addl_adv_manuf_data[0] = TYPE_LOCK_HOTEL_REMOTE<<2;
#endif
	m_addl_adv_manuf_data[0] |= (HW_VERSION&0x0C)>>2;
       m_addl_adv_manuf_data[1] = (HW_VERSION&0x03)<<6;
	m_addl_adv_manuf_data[1] |= (SW_VERSION&0x03F);   
	m_addl_adv_manuf_data[2] = 0;
	m_addl_adv_manuf_data[3] = 0;
	if(TouchTimer64ms != 0)
	{
		m_addl_adv_manuf_data[3] |= TOUCH_LIGHT_ON;
	}
	if(Sys_PataCfg.Pswd == 0x12345678)
	{
		m_addl_adv_manuf_data[3] |= INITIAL_STATUS;
	}
	if(Motor_PositStatus == MOTOR_POSIT_LOCK)
	{/* 1: LOCK   0:UNLOCK*/
		m_addl_adv_manuf_data[3] |= LOCK_STATUS;
	}
	if(Access_Record_Null == 0)
	{
		m_addl_adv_manuf_data[3] |= RECORD_STATUS;
	}
	m_addl_adv_manuf_data[4] = Access_BatteryData;
	memcpy(&m_addl_adv_manuf_data[5],m_mac_add,6);
	
}

extern ble_adv_mode_t                  m_adv_mode_current;
extern uint16_t                          m_conn_handle;
extern void advertising_init(void);

/****************************************************************************************************
**Function:
	void ble_dahao_start_advert(uint8 mode)
**Author: rory
**Description:
**Input: mode  0: touch keyboard  1: update ad status
**Output: 
****************************************************************************************************/
void ble_dahao_start_advert(uint8 mode)
{
	uint32_t err_code;
	
	if((m_ble_adv_timer != 0) || (m_conn_handle != BLE_CONN_HANDLE_INVALID))
	{
		if(mode == 1)
		{
			advertising_init();
		}
		return;
	}
	//return;
	if((m_ble_adv_status == BLE_ADV_STOP)  || (m_ble_adv_status == BLE_ADV_START_FAST))
	{
		advertising_init();
		err_code = ble_advertising_start(BLE_ADV_MODE_FAST);
    		APP_ERROR_CHECK(err_code);	
		m_ble_adv_timer = 500/64;
	}
	else if((m_ble_adv_status == BLE_ADV_SLOW) ||
		((m_ble_adv_status == BLE_ADV_FAST) && (mode == 1)))
	{
		advertising_init();
		err_code = sd_ble_gap_adv_stop(); //�ع㲥
		m_ble_adv_restart_timer = 300/64;
		APP_ERROR_CHECK(err_code);
		m_ble_adv_status = BLE_ADV_START_FAST;
		m_ble_adv_timer = 500/64;
	}
}

uint8 ble_dahao_stop_advert(void)
{
	uint32_t err_code;
	
	if(m_conn_handle != BLE_CONN_HANDLE_INVALID)
	{
		return E_NOT_OK;
	}
	err_code = sd_ble_gap_adv_stop(); //�ع㲥
	APP_ERROR_CHECK(err_code);
	return E_OK;
}

