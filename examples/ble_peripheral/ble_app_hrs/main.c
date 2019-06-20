/* Copyright (c) 2014 Nordic Semiconductor. All Rights Reserved.
 *
 * The information contained herein is property of Nordic Semiconductor ASA.
 * Terms and conditions of usage are described in detail in NORDIC
 * SEMICONDUCTOR STANDARD SOFTWARE LICENSE AGREEMENT.
 *
 * Licensees are granted free, non-transferable use of the information. NO
 * WARRANTY of ANY KIND is provided. This heading must NOT be removed from
 * the file.
 *
 */
/** @example examples/ble_peripheral/ble_app_hrs/main.c
 *
 * @brief Heart Rate Service Sample Application main file.
 *
 * This file contains the source code for a sample application using the Heart Rate service
 * (and also Battery and Device Information services). This application uses the
 * @ref srvlib_conn_params module.
 */

#include <stdint.h>
#include <string.h>
#include "nordic_common.h"
#include "nrf.h"
#include "app_error.h"
#include "ble.h"
#include "ble_hci.h"
#include "ble_srv_common.h"
#include "ble_advdata.h"
#include "ble_advertising.h"
//#include "ble_bas.h"
//#include "ble_hrs.h"
#include "ble_dahao.h"
//#include "ble_dis.h"
#ifdef BLE_DFU_APP_SUPPORT
#include "ble_dfu.h"
#include "dfu_app_handler.h"
#endif // BLE_DFU_APP_SUPPORT
#include "ble_conn_params.h"
#include "boards.h"
#include "sensorsim.h"
#include "softdevice_handler.h"
#include "app_timer.h"
#include "device_manager.h"
#include "pstorage.h"
#include "app_trace.h"
#include "bsp.h"
#include "nrf_delay.h"
#include "bsp_btn_ble.h"
#include "SYSTEM\sys.h"
#include "Beep\Beep.h"
#include "nrf_gpio.h"
#include "boards.h"
#include "Access\Access.h"
#include "Protocol\Proto_NetComm.h"
#include "RTC\Rtc.h"
#include "KEY\key.h"
#include "Card\Rc522.h"
#include "HC595\HC595.h"
#include "COMMU\Lock_Commu.h"
#include "Flash\Flash.h"
#include "SYSTEM\Sys_TimeBase.h"
#include "Light\light.h"
#include "Beep\Beep.h"
#include "KEY\Touch.h"
#include "protocol\Proto_Analy.h"
#include "lora\radio.h"
#include "KEY\Touch.h"
#include <cstdlib>
#include "Flash\Flash_Nrf.h"
#include "FACTORY\Factory.h"
#include "Motor\Motor.h"
#include "app_uart.h"


#define IS_SRVC_CHANGED_CHARACT_PRESENT  1                                          /**< Include or not the service_changed characteristic. if not enabled, the server's database cannot be changed for the lifetime of the device*/

#define CENTRAL_LINK_COUNT               0                                          /**< Number of central links used by the application. When changing this number remember to adjust the RAM settings*/
#define PERIPHERAL_LINK_COUNT            1                                          /**< Number of peripheral links used by the application. When changing this number remember to adjust the RAM settings*/

#define DEVICE_NAME                      "YYLOCK"                               /**< Name of device. Will be included in the advertising data. */
#define MANUFACTURER_NAME                "DH"                      /**< Manufacturer. Will be passed to Device Information Service. */
#define APP_ADV_INTERVAL                 160                                        /**< The advertising interval (in units of 0.625 ms. This value corresponds to 25 ms). */
#define APP_ADV_INTERVAL_ALL          160*9   // 160*9                                        /**< The advertising interval (in units of 0.625 ms. This value corresponds to 25 ms). */
#define APP_ADV_TIMEOUT_IN_SECONDS       0                                        /**< The advertising timeout in units of seconds. */

#define APP_TIMER_PRESCALER              0                                          /**< Value of the RTC1 PRESCALER register. */
#define APP_TIMER_OP_QUEUE_SIZE          6                                          /**< Size of timer operation queues. */

#define MIN_CONN_INTERVAL               MSEC_TO_UNITS(60, UNIT_1_25_MS) //MSEC_TO_UNITS(100, UNIT_1_25_MS)//MSEC_TO_UNITS(400, UNIT_1_25_MS)           /**< Minimum acceptable connection interval (0.4 seconds). */
#define MAX_CONN_INTERVAL               MSEC_TO_UNITS(100, UNIT_1_25_MS)// MSEC_TO_UNITS(200, UNIT_1_25_MS)//MSEC_TO_UNITS(650, UNIT_1_25_MS)           /**< Maximum acceptable connection interval (0.65 second). */
#define SLAVE_LATENCY                    0                                          /**< Slave latency. */
#define CONN_SUP_TIMEOUT                 MSEC_TO_UNITS(4000, UNIT_10_MS)            /**< Connection supervisory timeout (4 seconds). */

#define FIRST_CONN_PARAMS_UPDATE_DELAY   APP_TIMER_TICKS(5000, APP_TIMER_PRESCALER) /**< Time from initiating event (connect or start of notification) to first time sd_ble_gap_conn_param_update is called (5 seconds). */
#define NEXT_CONN_PARAMS_UPDATE_DELAY    APP_TIMER_TICKS(30000, APP_TIMER_PRESCALER)/**< Time between each call to sd_ble_gap_conn_param_update after the first call (30 seconds). */
#define MAX_CONN_PARAMS_UPDATE_COUNT     3                                          /**< Number of attempts before giving up the connection parameter negotiation. */

#define SEC_PARAM_BOND                   1                                          /**< Perform bonding. */
#define SEC_PARAM_MITM                   0                                          /**< Man In The Middle protection not required. */
#define SEC_PARAM_LESC                   0                                          /**< LE Secure Connections not enabled. */
#define SEC_PARAM_KEYPRESS               0                                          /**< Keypress notifications not enabled. */
#define SEC_PARAM_IO_CAPABILITIES        BLE_GAP_IO_CAPS_NONE                       /**< No I/O capabilities. */
#define SEC_PARAM_OOB                    0                                          /**< Out Of Band data not available. */
#define SEC_PARAM_MIN_KEY_SIZE           7                                          /**< Minimum encryption key size. */
#define SEC_PARAM_MAX_KEY_SIZE           16                                         /**< Maximum encryption key size. */

#define DEAD_BEEF                        0xDEADBEEF                                 /**< Value used as error code on stack dump, can be used to identify stack location on stack unwind. */
#ifdef BLE_DFU_APP_SUPPORT
#define DFU_REV_MAJOR                    0x00                                       /** DFU Major revision number to be exposed. */
#define DFU_REV_MINOR                    0x01                                       /** DFU Minor revision number to be exposed. */
#define DFU_REVISION                     ((DFU_REV_MAJOR << 8) | DFU_REV_MINOR)     /** DFU Revision number to be exposed. Combined of major and minor versions. */
#define APP_SERVICE_HANDLE_START         0x000C                                     /**< Handle of first application specific service when when service changed characteristic is present. */
#define BLE_HANDLE_MAX                   0xFFFF                                     /**< Max handle value in BLE. */

STATIC_ASSERT(IS_SRVC_CHANGED_CHARACT_PRESENT);                                     /** When having DFU Service support in application the Service Changed Characteristic should always be present. */
#endif // BLE_DFU_APP_SUPPORT


/*static */ uint16_t                          m_conn_handle = BLE_CONN_HANDLE_INVALID;   /**< Handle of the current connection. */
ble_dahao_t                        m_ble_dahao;

//APP_TIMER_DEF(LoraWakeup);                                                  			/**< Battery timer. */
//APP_TIMER_DEF(LoraEnsleep);                                                  			/**< Battery timer. */
//APP_TIMER_DEF(LoraDetBeacon);                                                  			/**< Battery timer. */
//APP_TIMER_DEF(LoraCallBack);                                                  			/**< Battery timer. */
//APP_TIMER_DEF(LoraSend);                                                  			/**< Battery timer. */

APP_TIMER_DEF(carddet);
APP_TIMER_DEF(SysBase);   
//APP_TIMER_DEF(TimeOoutQuic);   

APP_TIMER_DEF(BleProc);                                                  			
APP_TIMER_DEF(BattDet);                                                  			



static dm_application_instance_t         m_app_handle;                              /**< Application identifier allocated by device manager */

static ble_uuid_t m_adv_uuids[] = {{BLE_UUID_DAHAO_SERVICE,         BLE_UUID_TYPE_BLE},
                                  // {BLE_UUID_BATTERY_SERVICE,            BLE_UUID_TYPE_BLE},
                                  /*{BLE_UUID_DEVICE_INFORMATION_SERVICE, BLE_UUID_TYPE_BLE}*/}; /**< Universally unique service identifiers. */
#ifdef BLE_DFU_APP_SUPPORT
static ble_dfu_t                         m_dfus;                                    /**< Structure used to identify the DFU service. */
#endif // BLE_DFU_APP_SUPPORT
extern Std_ReturnType power_out_sleep(void);
extern void Lora_timer_start(uint8 sttype,uint16 time);
extern void Carddet_timer_start(uint16 time);

extern void Tsmxx_Irq_Init(void);


uint8  Lora_SendTimes = 0;
uint8  Lora_State;
uint8  Lora_CadCount;
Proto_NetcommType Proto_NetSendBack = NET_NULL;
uint8  Lorasendx64ms= 0;
uint8 SysSleepFlag = STD_FALSE;
uint8 BattdetTimer = STD_FALSE;

extern uint32 Sys_RestTimex64ms;
extern uint8  protoAnaly_freqsetflag;

/**@brief Function for the Power manager.
 */
static void power_manage(void)
{
    uint32_t err_code = sd_app_evt_wait();
    APP_ERROR_CHECK(err_code);
}

/**@brief Callback function for asserts in the SoftDevice.
 *
 * @details This function will be called in case of an assert in the SoftDevice.
 *
 * @warning This handler is an example only and does not fit a final product. You need to analyze
 *          how your product is supposed to react in case of Assert.
 * @warning On assert from the SoftDevice, the system can only recover on reset.
 *
 * @param[in] line_num   Line number of the failing ASSERT call.
 * @param[in] file_name  File name of the failing ASSERT call.
 */
void assert_nrf_callback(uint16_t line_num, const uint8_t * p_file_name)
{
    app_error_handler(DEAD_BEEF, line_num, p_file_name);
}

void ble_dahao_timer_proc(void)
{
	uint32_t                err_code;
	
	if(Sys_DisableBlueTooth())
	{
		return;
	}
	if(m_disconnect_timer != 0)
	{
		--m_disconnect_timer;
		if(m_disconnect_timer == 0)
		{
			g_LockComuData.recIndex = 0;
			 if (m_conn_handle != BLE_CONN_HANDLE_INVALID)
	            {
	                err_code = sd_ble_gap_disconnect(m_conn_handle,
	                                                 BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
	                APP_ERROR_CHECK(err_code);
	            }
		}
	}
	if(m_ble_adv_timer != 0)
	{
		m_ble_adv_timer--;
	}
	if(m_ble_adv_restart_timer != 0)
	{
		m_ble_adv_restart_timer--;
		if((m_ble_adv_restart_timer == 0) && (m_conn_handle == BLE_CONN_HANDLE_INVALID)
			&& ((m_ble_adv_status != BLE_ADV_FAST) && (m_ble_adv_status != BLE_ADV_SLOW)))
		{
			err_code = ble_advertising_start(BLE_ADV_MODE_FAST);
    			APP_ERROR_CHECK(err_code);	
			m_ble_adv_timer =500/64;
		}
	}
}


void SendUart_data(uint8_t * p_data, uint16_t length)
{
    for (uint32_t i = 0; i < length; i++)
    {
        while (app_uart_put(p_data[i]) != NRF_SUCCESS);
    }
}


void BleProc_timer_start(uint16 time)
{
	app_timer_stop(BleProc);
	app_timer_start(BleProc, APP_TIMER_TICKS(time, APP_TIMER_PRESCALER), NULL);
}
static void BleProc_timer_handler(void * p_context)
{
	LockCommu_Proc();
}

void SysBasetimer_Start(uint16 time)
{	
	app_timer_stop(SysBase);
	app_timer_start(SysBase, APP_TIMER_TICKS(time, APP_TIMER_PRESCALER), NULL);
	SysSleepFlag = STD_FALSE;
}


uint8_t wdat[4] = {0x11,0x22,0x33,0x44};

static void SysBase_timer_handler(void * p_context)
{	
#if 0
	if(Sys_State == SYS_SLEEP)
	{
		SysBasetimer_Start(60000);
		if(Sys_RestTimex64ms > 60000/64)
		{
			Sys_RestTimex64ms-= 60000/64;
		}
		else if(Sys_RestTimex64ms != 0)
		{
			Sys_RestTimex64ms = 0;		
			Sys_RstTimsoutx64ms = 1000/64;
		}
		return ;
	}
#endif	

	ReLay_Timepro();

	Touch_TimerProc();
	KeyDrive.ioctl(KEY_SCAN,NULL);
	Touch_Proc();
	ble_dahao_timer_proc();
	Access_TimerProc();
	//SendUart_data(wdat,4);
	//Send_Weigand34(&wdat);
	
	//HC595_LedWriteData(0x1000);
	
	Sys_RestProc();
#if ((defined USE_FLASH_NRF) && (USE_FLASH_NRF == STD_TRUE))
	NrfFlashDrive.ioctl(NRFFLASH_SCAN, NULL);
#endif
	Factory_Proc();
	
	SysBasetimer_Start(64);	
#if 0	
	if(Sys_RestTimex64ms)
	{
		Sys_RestTimex64ms--;
		if(Sys_RestTimex64ms ==0)
		{
			Sys_RstTimsoutx64ms = 1000/64;
		}
	}
#endif
}
uint32 cntkk;
void Carddet_timer_start(uint16 time)
{	
	app_timer_stop(carddet);
	app_timer_start(carddet, APP_TIMER_TICKS(time, APP_TIMER_PRESCALER), NULL);
	SysSleepFlag = STD_FALSE;
}

static void Carddet_timer_handler(void * p_context)
{	
	if(Sys_CheckCardSleepDet() == 0)
	{
		app_timer_stop(carddet);
		app_timer_start(carddet, APP_TIMER_TICKS(1000, APP_TIMER_PRESCALER), NULL);
	}

	//if((SysSleepFlag == STD_TRUE)&&(Sys_CheckCardSleepDet() == 0))
	//{
	//	return ;
	//}
	
#if (defined WATCHDOG_EN) && (WATCHDOG_EN == STD_TRUE)
//		Sys_wdt_feed();
#endif

	//cntkk++;

	//Access_BeepStart(BEEP_NORMAL,1);	


    if ( cardopenFlag )   return ;
	Sys_SpiOpen();
	Access_DetCardProc();
	//Sys_SpiClose();
}

#if 0
void Battdet_timer_start(uint16 time)
{	
	if(BattdetTimer == STD_TRUE)
	{
		return;
	}
	BattdetTimer = STD_TRUE;
	Vdd_PowerOn();
	app_timer_stop(BattDet);
	app_timer_start(BattDet, APP_TIMER_TICKS(time, APP_TIMER_PRESCALER), NULL);
	SysSleepFlag = STD_FALSE;
}

static void Battdet_timer_handler(void * p_context)
{		
	BattdetTimer = STD_FALSE;
#if (defined LORA_ENABLE) && (LORA_ENABLE == STD_TRUE)
	if(loraworkmode != RADIO_SET_SLEEP)	
	{
		Battdet_timer_start(100);
		return;
	}
#endif
	if(Access_MotorTimer64 != 0)
	{
		Battdet_timer_start(200);
		return;
	}
	Access_BatteryData= Sys_GetBat();	
	if((Access_LockTimer64 == 0)&&(Access_MotorTimer64 == 0))
	{
		Vdd_PowerOff();
	}
}
#endif
/**@brief Function for the Timer initialization.
 *
 * @details Initializes the timer module. This creates and starts application timers.
 */
static void timers_init(void)
{
   uint32_t err_code;

// Initialize timer module.
    APP_TIMER_INIT(APP_TIMER_PRESCALER, APP_TIMER_OP_QUEUE_SIZE, false);
// Create timers.
	err_code = app_timer_create(&BleProc, APP_TIMER_MODE_SINGLE_SHOT, BleProc_timer_handler);
	err_code = app_timer_create(&SysBase, APP_TIMER_MODE_SINGLE_SHOT, SysBase_timer_handler);
	err_code = app_timer_create(&carddet, APP_TIMER_MODE_SINGLE_SHOT, Carddet_timer_handler);
	//err_code = app_timer_create(&BattDet, APP_TIMER_MODE_SINGLE_SHOT, Battdet_timer_handler);
}

/**@brief Function for the GAP initialization.
 *
 * @details This function sets up all the necessary GAP (Generic Access Profile) parameters of the
 *			device including the device name, appearance, and the preferred connection parameters.
 */
static void gap_params_init(void)
{
	uint32_t				err_code;
	ble_gap_conn_params_t	gap_conn_params;
	ble_gap_conn_sec_mode_t sec_mode;

	BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);


    ble_dahao_get_scan_data();
    err_code = sd_ble_gap_device_name_set(&sec_mode,
                                          (const uint8_t *) m_addl_adv_data,
                                          strlen(m_addl_adv_data));
    APP_ERROR_CHECK(err_code);
    memset(&gap_conn_params, 0, sizeof(gap_conn_params));

    gap_conn_params.min_conn_interval = MIN_CONN_INTERVAL;
    gap_conn_params.max_conn_interval = MAX_CONN_INTERVAL;
    gap_conn_params.slave_latency     = SLAVE_LATENCY;
    gap_conn_params.conn_sup_timeout  = CONN_SUP_TIMEOUT;

    err_code = sd_ble_gap_ppcp_set(&gap_conn_params);
    APP_ERROR_CHECK(err_code);

    err_code = sd_ble_gap_tx_power_set(Sys_PataCfg.bleCfg.txPower); 
    APP_ERROR_CHECK(err_code);
}


#ifdef BLE_DFU_APP_SUPPORT
/**@brief Function for stopping advertising.
 */
static void advertising_stop(void)
{
    uint32_t err_code;

    err_code = sd_ble_gap_adv_stop();
    APP_ERROR_CHECK(err_code);

 /*   err_code = bsp_indication_set(BSP_INDICATE_IDLE);
    APP_ERROR_CHECK(err_code);*/
}


/**@brief Function for loading application-specific context after establishing a secure connection.
 *
 * @details This function will load the application context and check if the ATT table is marked as
 *          changed. If the ATT table is marked as changed, a Service Changed Indication
 *          is sent to the peer if the Service Changed CCCD is set to indicate.
 *
 * @param[in] p_handle The Device Manager handle that identifies the connection for which the context
 *                     should be loaded.
 */
static void app_context_load(dm_handle_t const * p_handle)
{
    uint32_t                 err_code;
    static uint32_t          context_data;
    dm_application_context_t context;

    context.len    = sizeof(context_data);
    context.p_data = (uint8_t *)&context_data;

    err_code = dm_application_context_get(p_handle, &context);
    if (err_code == NRF_SUCCESS)
    {
        // Send Service Changed Indication if ATT table has changed.
        if ((context_data & (DFU_APP_ATT_TABLE_CHANGED << DFU_APP_ATT_TABLE_POS)) != 0)
        {
            err_code = sd_ble_gatts_service_changed(m_conn_handle, APP_SERVICE_HANDLE_START, BLE_HANDLE_MAX);
            if ((err_code != NRF_SUCCESS) &&
                (err_code != BLE_ERROR_INVALID_CONN_HANDLE) &&
                (err_code != NRF_ERROR_INVALID_STATE) &&
                (err_code != BLE_ERROR_NO_TX_PACKETS) &&
                (err_code != NRF_ERROR_BUSY) &&
                (err_code != BLE_ERROR_GATTS_SYS_ATTR_MISSING))
            {
                APP_ERROR_HANDLER(err_code);
            }
        }

        err_code = dm_application_context_delete(p_handle);
        APP_ERROR_CHECK(err_code);
    }
    else if (err_code == DM_NO_APP_CONTEXT)
    {
        // No context available. Ignore.
    }
    else
    {
        APP_ERROR_HANDLER(err_code);
    }
}


/** @snippet [DFU BLE Reset prepare] */
/**@brief Function for preparing for system reset.
 *
 * @details This function implements @ref dfu_app_reset_prepare_t. It will be called by
 *          @ref dfu_app_handler.c before entering the bootloader/DFU.
 *          This allows the current running application to shut down gracefully.
 */
static void reset_prepare(void)
{
    uint32_t err_code;

    if (m_conn_handle != BLE_CONN_HANDLE_INVALID)
    {
        // Disconnect from peer.
        err_code = sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
        APP_ERROR_CHECK(err_code);
       /* err_code = bsp_indication_set(BSP_INDICATE_IDLE);
        APP_ERROR_CHECK(err_code);*/
    }
    else
    {
        // If not connected, the device will be advertising. Hence stop the advertising.
        advertising_stop();
    }

    err_code = ble_conn_params_stop();
    APP_ERROR_CHECK(err_code);

    nrf_delay_ms(500);
}
/** @snippet [DFU BLE Reset prepare] */
#endif // BLE_DFU_APP_SUPPORT


/**@brief Function for initializing services that will be used by the application.
 *
 * @details Initialize the Heart Rate, Battery and Device Information services.
 */
static void services_init(void)
{
    uint32_t       err_code;
    //ble_bas_init_t bas_init;
    //ble_dis_init_t dis_init;

    err_code = ble_dahao_add_service(&m_ble_dahao);
    APP_ERROR_CHECK(err_code);
    err_code = ble_dahao_add_characteristics(&m_ble_dahao);
    APP_ERROR_CHECK(err_code);
#ifdef BLE_DFU_APP_SUPPORT
    /** @snippet [DFU BLE Service initialization] */
    ble_dfu_init_t   dfus_init;

    // Initialize the Device Firmware Update Service.
    memset(&dfus_init, 0, sizeof(dfus_init));

    dfus_init.evt_handler   = dfu_app_on_dfu_evt;
    dfus_init.error_handler = NULL;
    dfus_init.evt_handler   = dfu_app_on_dfu_evt;
    dfus_init.revision      = DFU_REVISION;

    err_code = ble_dfu_init(&m_dfus, &dfus_init);
    APP_ERROR_CHECK(err_code);

    dfu_app_reset_prepare_set(reset_prepare);
    dfu_app_dm_appl_instance_set(m_app_handle);
    /** @snippet [DFU BLE Service initialization] */
#endif // BLE_DFU_APP_SUPPORT
}


/**@brief Function for handling the Connection Parameters Module.
 *
 * @details This function will be called for all events in the Connection Parameters Module which
 *          are passed to the application.
 *          @note All this function does is to disconnect. This could have been done by simply
 *                setting the disconnect_on_fail config parameter, but instead we use the event
 *                handler mechanism to demonstrate its use.
 *
 * @param[in] p_evt  Event received from the Connection Parameters Module.
 */
static void on_conn_params_evt(ble_conn_params_evt_t * p_evt)
{
    uint32_t err_code;

    if (p_evt->evt_type == BLE_CONN_PARAMS_EVT_FAILED)
    {
        err_code = sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_CONN_INTERVAL_UNACCEPTABLE);
        APP_ERROR_CHECK(err_code);
    }
}


/**@brief Function for handling a Connection Parameters error.
 *
 * @param[in] nrf_error  Error code containing information about what went wrong.
 */
static void conn_params_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}


/**@brief Function for initializing the Connection Parameters module.
 */
static void conn_params_init(void)
{
    uint32_t               err_code;
    ble_conn_params_init_t cp_init;

    memset(&cp_init, 0, sizeof(cp_init));
#if 1
    cp_init.p_conn_params                  = NULL;
    cp_init.first_conn_params_update_delay = FIRST_CONN_PARAMS_UPDATE_DELAY;
    cp_init.next_conn_params_update_delay  = NEXT_CONN_PARAMS_UPDATE_DELAY;
    cp_init.max_conn_params_update_count   = MAX_CONN_PARAMS_UPDATE_COUNT;
    cp_init.start_on_notify_cccd_handle    = BLE_GATT_HANDLE_INVALID;
    cp_init.disconnect_on_fail             = false;
    cp_init.evt_handler                    = on_conn_params_evt;
    cp_init.error_handler                  = conn_params_error_handler;

    err_code = ble_conn_params_init(&cp_init);
    APP_ERROR_CHECK(err_code);
#else
    cp_init.p_conn_params                  = NULL;
    cp_init.first_conn_params_update_delay = FIRST_CONN_PARAMS_UPDATE_DELAY;
    cp_init.next_conn_params_update_delay  = NEXT_CONN_PARAMS_UPDATE_DELAY;
    cp_init.max_conn_params_update_count   = MAX_CONN_PARAMS_UPDATE_COUNT;
    cp_init.start_on_notify_cccd_handle    = m_hrs.hrm_handles.cccd_handle;
    cp_init.disconnect_on_fail             = false;
    cp_init.evt_handler                    = on_conn_params_evt;
    cp_init.error_handler                  = conn_params_error_handler;

    err_code = ble_conn_params_init(&cp_init);
    APP_ERROR_CHECK(err_code);
#endif
}


/**@brief Function for putting the chip into sleep mode.
 *
 * @note This function will not return.
 */
static void sleep_mode_enter(void)
{
    uint32_t err_code;// = bsp_indication_set(BSP_INDICATE_IDLE);
  //  APP_ERROR_CHECK(err_code);

    // Prepare wakeup buttons.
    err_code = bsp_btn_ble_sleep_mode_prepare();
    APP_ERROR_CHECK(err_code);
    return ;
    // Go to system-off mode (this function will not return; wakeup will cause a reset).
   /* err_code = sd_power_system_off();
    APP_ERROR_CHECK(err_code);*/
}


/**@brief Function for handling advertising events.
 *
 * @details This function will be called for advertising events which are passed to the application.
 *
 * @param[in] ble_adv_evt  Advertising event.
 */
static void on_adv_evt(ble_adv_evt_t ble_adv_evt)
{
    switch (ble_adv_evt)
    {
        case BLE_ADV_EVT_FAST:
		m_ble_adv_status = BLE_ADV_FAST;
           m_ble_adv_timer = 500/64;
            break;	
	case BLE_ADV_EVT_SLOW:
           m_ble_adv_status = BLE_ADV_SLOW;
           m_ble_adv_timer = 500/64;
            break;
        case BLE_ADV_EVT_IDLE:
	    sleep_mode_enter();
           m_ble_adv_status = BLE_ADV_STOP;
            break;
        default:
            break;
    }
}


/**@brief Function for handling the Application's BLE Stack events.
 *
 * @param[in] p_ble_evt  Bluetooth stack event.
 */


//Á´½Ó»½ÐÑ
static void on_ble_evt(ble_evt_t * p_ble_evt)
{
    //uint32_t err_code;

    switch (p_ble_evt->header.evt_id)
            {
        case BLE_GAP_EVT_CONNECTED:
           /* err_code = bsp_indication_set(BSP_INDICATE_CONNECTED);
            APP_ERROR_CHECK(err_code);*/
            m_conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
		   
	    BLE_UPDATA_DISCONNECT_TIMER(Sys_PataCfg.bleCfg.connectOff/64);
	    //power_out_sleep();
	     //m_disconnect_timer = Sys_PataCfg.bleCfg.connectOff/64;
            break;

        case BLE_GAP_EVT_DISCONNECTED:
            m_conn_handle = BLE_CONN_HANDLE_INVALID;
            break;
        default:
            // No implementation needed.
            break;
    }
}


/**@brief Function for dispatching a BLE stack event to all modules with a BLE stack event handler.
 *
 * @details This function is called from the BLE Stack event interrupt handler after a BLE stack
 *          event has been received.
 *
 * @param[in] p_ble_evt  Bluetooth stack event.
 */
static void ble_evt_dispatch(ble_evt_t * p_ble_evt)
{
    dm_ble_evt_handler(p_ble_evt);
   // ble_hrs_on_ble_evt(&m_hrs, p_ble_evt);
   ble_dahao_on_ble_evt(&m_ble_dahao, p_ble_evt);
    //ble_bas_on_ble_evt(&m_bas, p_ble_evt);
    ble_conn_params_on_ble_evt(p_ble_evt);
    //bsp_btn_ble_on_ble_evt(p_ble_evt);
#ifdef BLE_DFU_APP_SUPPORT
    /** @snippet [Propagating BLE Stack events to DFU Service] */
    ble_dfu_on_ble_evt(&m_dfus, p_ble_evt);
    /** @snippet [Propagating BLE Stack events to DFU Service] */
#endif // BLE_DFU_APP_SUPPORT
    on_ble_evt(p_ble_evt);
    ble_advertising_on_ble_evt(p_ble_evt);
}


/**@brief Function for dispatching a system event to interested modules.
 *
 * @details This function is called from the System event interrupt handler after a system
 *          event has been received.
 *
 * @param[in] sys_evt  System stack event.
 */
static void sys_evt_dispatch(uint32_t sys_evt)
{
    pstorage_sys_event_handler(sys_evt);
    ble_advertising_on_sys_evt(sys_evt);
}


/**@brief Function for initializing the BLE stack.
 *
 * @details Initializes the SoftDevice and the BLE event interrupt.
 */
static void ble_stack_init(void)
{
    uint32_t err_code;
    
    nrf_clock_lf_cfg_t clock_lf_cfg = NRF_CLOCK_LFCLKSRC;
    
    // Initialize the SoftDevice handler module.	
    SOFTDEVICE_HANDLER_INIT(&clock_lf_cfg, NULL);

    ble_enable_params_t ble_enable_params;
    err_code = softdevice_enable_get_default_config(CENTRAL_LINK_COUNT,
                                                    PERIPHERAL_LINK_COUNT,
                                                    &ble_enable_params);
    APP_ERROR_CHECK(err_code);

#ifdef BLE_DFU_APP_SUPPORT
    ble_enable_params.gatts_enable_params.service_changed = 1;
#endif // BLE_DFU_APP_SUPPORT
    //Check the ram settings against the used number of links
    CHECK_RAM_START_ADDR(CENTRAL_LINK_COUNT,PERIPHERAL_LINK_COUNT);

    // Enable BLE stack.
    err_code = softdevice_enable(&ble_enable_params);
    APP_ERROR_CHECK(err_code);

    // Register with the SoftDevice handler module for BLE events.
    err_code = softdevice_ble_evt_handler_set(ble_evt_dispatch);
    APP_ERROR_CHECK(err_code);

    // Register with the SoftDevice handler module for BLE events.
    err_code = softdevice_sys_evt_handler_set(sys_evt_dispatch);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for handling events from the BSP module.
 *
 * @param[in]   event   Event generated by button press.
 */
void bsp_event_handler(bsp_event_t event)
{
    uint32_t err_code;
    switch (event)
    {
        case BSP_EVENT_SLEEP:
            sleep_mode_enter();
            break;

        case BSP_EVENT_DISCONNECT:
            err_code = sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            if (err_code != NRF_ERROR_INVALID_STATE)
            {
                APP_ERROR_CHECK(err_code);
            }
            break;

        case BSP_EVENT_WHITELIST_OFF:
            err_code = ble_advertising_restart_without_whitelist();
            if (err_code != NRF_ERROR_INVALID_STATE)
            {
                APP_ERROR_CHECK(err_code);
            }
            break;

        default:
            break;
    }
}


/**@brief Function for handling the Device Manager events.
 *
 * @param[in] p_evt  Data associated to the device manager event.
 */
static uint32_t device_manager_evt_handler(dm_handle_t const * p_handle,
                                           dm_event_t const  * p_event,
                                           ret_code_t        event_result)
{
    APP_ERROR_CHECK(event_result);

#ifdef BLE_DFU_APP_SUPPORT
    if (p_event->event_id == DM_EVT_LINK_SECURED)
    {
        app_context_load(p_handle);
    }
#endif // BLE_DFU_APP_SUPPORT

    return NRF_SUCCESS;
}


/**@brief Function for the Device Manager initialization.
 *
 * @param[in] erase_bonds  Indicates whether bonding information should be cleared from
 *                         persistent storage during initialization of the Device Manager.
 */
static void device_manager_init(bool erase_bonds)
{
    uint32_t               err_code;
    dm_init_param_t        init_param = {.clear_persistent_data = erase_bonds};
    dm_application_param_t register_param;

    // Initialize persistent storage module.
    err_code = pstorage_init();
    APP_ERROR_CHECK(err_code);

    err_code = dm_init(&init_param);
    APP_ERROR_CHECK(err_code);

    memset(&register_param.sec_param, 0, sizeof(ble_gap_sec_params_t));

    register_param.sec_param.bond         = SEC_PARAM_BOND;
    register_param.sec_param.mitm         = SEC_PARAM_MITM;
    register_param.sec_param.lesc         = SEC_PARAM_LESC;
    register_param.sec_param.keypress     = SEC_PARAM_KEYPRESS;
    register_param.sec_param.io_caps      = SEC_PARAM_IO_CAPABILITIES;
    register_param.sec_param.oob          = SEC_PARAM_OOB;
    register_param.sec_param.min_key_size = SEC_PARAM_MIN_KEY_SIZE;
    register_param.sec_param.max_key_size = SEC_PARAM_MAX_KEY_SIZE;
    register_param.evt_handler            = device_manager_evt_handler;
    register_param.service_type           = DM_PROTOCOL_CNTXT_GATT_SRVR_ID;

    err_code = dm_register(&m_app_handle, &register_param);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for initializing the Advertising functionality.
 */
void advertising_init(void)
{
    uint32_t      err_code;
    ble_advdata_t advdata;
    ble_advdata_t scanrsp;
    ble_advdata_manuf_data_t manuf_data;

    // Build advertising data struct to pass into @ref ble_advertising_init.
    memset(&advdata, 0, sizeof(advdata));

    advdata.name_type               = BLE_ADVDATA_FULL_NAME;
    advdata.include_appearance      = false;
    advdata.flags                   = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;
    advdata.uuids_complete.uuid_cnt = sizeof(m_adv_uuids) / sizeof(m_adv_uuids[0]);
    advdata.uuids_complete.p_uuids  = m_adv_uuids;

    ble_adv_modes_config_t options = {0};
    options.ble_adv_fast_enabled  = BLE_ADV_FAST_ENABLED;
    options.ble_adv_slow_enabled  = BLE_ADV_SLOW_ENABLED;
    if(Sys_CheckBTSleepMode())
    {
     options.ble_adv_fast_interval = APP_ADV_INTERVAL;
		 options.ble_adv_fast_timeout  = 15;
		 options.ble_adv_slow_interval = APP_ADV_INTERVAL*2;
		 options.ble_adv_slow_timeout  = 15;
    }
    else
    {
     options.ble_adv_fast_interval = APP_ADV_INTERVAL;
		 options.ble_adv_fast_timeout  = 15;
    		
		 options.ble_adv_slow_interval = APP_ADV_INTERVAL_ALL;
		 options.ble_adv_slow_timeout  = 0;
    }
   
    ble_dahao_get_manual_data();
    manuf_data.company_identifier = 0x0059;
    manuf_data.data.p_data = m_addl_adv_manuf_data;
    manuf_data.data.size = BLE_MANUF_DATA_LEN;

    memset(&scanrsp, 0, sizeof(scanrsp));
    scanrsp.p_manuf_specific_data   = &manuf_data;
    err_code = ble_advertising_init(&advdata, &scanrsp, &options, on_adv_evt, NULL);

    APP_ERROR_CHECK(err_code);
}

/****************************************************************************************************
**Function:
	Std_ReturnType power_out_sleep(void)
**Author: rory
**Description:
**Input: 
**Output: 
****************************************************************************************************/
void power_enter_sleep(void)
{
	m_ble_adv_timer = 0;
}


uint32 rtc1timebak;

/****************************************************************************************************
**Function:
	Std_ReturnType Access_GetSupportType(uint8 idtype)
**Author: rory
**Description:
**Input: 
**Output: 
****************************************************************************************************/
Std_ReturnType power_enter_sleepdet(void)
{
    static  uint32 cntrtc,cntwdt;
    if ( ++cntwdt >= 250000 )
      {
        cntwdt = 0;
        #if (defined WATCHDOG_EN) && (WATCHDOG_EN == STD_TRUE)
		Sys_wdt_feed();
        #endif
      }
 
    if ( ++cntrtc >= 1000000 )
      {
         cntrtc = 0;

		 if ( rtc1timebak == NRF_RTC1->CC[0] )
		   {
		     NVIC_SystemReset();
		   }
		 else
		   {
		     rtc1timebak = NRF_RTC1->CC[0];
		   }
      }

#if 0
	if((BeepDrive.ioctl(BEEP_CMD_GET_STATE,NULL) != E_OK)||(LightDrive.ioctl(LIGHT_CMD_GET_STATE,NULL) != E_OK)
	||(m_disconnect_timer != 0)||/*(Access_MotorTimer64 != 0) ||(Access_LockTimer64 != 0)||*/(Access_CardLearnTimer64 != 0)
	||(Access_DefaultTimer64 != 0)||(Access_LockDeviceTimer64ms != 0)
#if (defined TOUCH_ENABLE) && (TOUCH_ENABLE == STD_TRUE)	
	||(TouchTimer64ms != 0)
#endif
	||(Sys_StoreTimsoutx64ms != 0) || (Sys_RstTimsoutx64ms != 0) || (m_ble_adv_restart_timer != 0)
       ||(Sys_McuRestFlag != STD_FALSE) || (Motor_State != MOTOR_STAT_IDLE)||(Motor_AutoLockTimer64ms!=0)
       ||(Factory_State != COMD_FATORY_IDLE))
	{
		if(sysbasewkflag == STD_FALSE)
		{
			sysbasewkflag= STD_TRUE;
			SysBasetimer_Start(10);
		}
		Sys_State = SYS_NORMAL;
	}
	else
	{
		sysbasewkflag= STD_FALSE;
		SysSleepFlag = STD_TRUE;
		Sys_State = SYS_SLEEP;
#if (defined TOUCH_ENABLE) && (TOUCH_ENABLE == STD_TRUE)
		if(TouchRstTimes == 1)
		{
			TouchRstTimes = 0;
			Tsmxx_Open();
		}
		else
		{
			Tsmxx_Irq_Init();
		}
#endif
	//	Hc595_PowerOff();
	//	Vdd_PowerOff();
      Sys_SleepErrTimsoutx64ms = 0;
	}
	
	//power_enter_sleep();
	//power_manage();
	//Sys_WkupType = 0;
#if (defined WATCHDOG_EN) && (WATCHDOG_EN == STD_TRUE)

#endif
#endif	

	return E_OK;
}

/****************************************************************************************************
**Function:
	Std_ReturnType power_out_sleep(void)
**Author: rory
**Description:
**Input: 
**Output: 
****************************************************************************************************/
Std_ReturnType power_out_sleep(void)
{
	Vdd_PowerOn();
	return E_OK;
}

#define UART_TX_BUF_SIZE 64                         /**< UART TX buffer size. */
#define UART_RX_BUF_SIZE 64     

uint8_t   uartBuff[2]={0};  
uint16_t  uartLenx =0;  
uint8_t   uartNodat=0;

void uart_event_handle(app_uart_evt_t * p_event)  
{
    if (p_event->evt_type == APP_UART_DATA_READY)  
    {  
        app_uart_get(&uartBuff[uartLenx++]); 
			  uartNodat=0;
	}
}

void uart_init(void)
{
    uint32_t err_code;
    const app_uart_comm_params_t comm_params =
    {
         29,//RX_PIN_NUMBER,
         28,//TX_PIN_NUMBER,
         29,//RTS_PIN_NUMBER,
         28,//CTS_PIN_NUMBER,
         APP_UART_FLOW_CONTROL_DISABLED,
         false,
         UART_BAUDRATE_BAUDRATE_Baud115200
    };

    APP_UART_FIFO_INIT(&comm_params,
						 UART_RX_BUF_SIZE,
						 UART_TX_BUF_SIZE,
			             uart_event_handle,
                         APP_IRQ_PRIORITY_LOW,
                         err_code);

    APP_ERROR_CHECK(err_code);
}

#if 0
#define S_SWAP(a,b) do { uint8_t t = S[a]; S[a] = S[b]; S[b] = t; } while(0)

int rc4_skip(const uint8_t *key, uint32_t keylen, uint32_t skip,uint8_t *data, uint32_t data_len)
{	
    uint32_t i, j, k;	
    uint8_t S[256], *pos;	
    uint32_t kpos;	
    
    /* Setup RC4 state */	
    for (i = 0; i < 256; i++)
    {
        S[i] = i;
    }
    
    j = 0;	
    kpos = 0;	
    for (i = 0; i < 256; i++) 
    {		
        j = (j + S[i] + key[kpos]) & 0xff;
        
        kpos++;		
        if (kpos >= keylen)
        {
            kpos = 0;
        }
        S_SWAP(i, j);
    }	
    
    /* Skip the start of the stream */	
    i = j = 0;	
    for (k = 0; k < skip; k++) 
    {		
        i = (i + 1) & 0xff;		
        j = (j + S[i]) & 0xff;		
        S_SWAP(i, j);
    }	
    
    /* Apply RC4 to data */	
    pos = data;	
    for (k = 0; k < data_len; k++) 
    {		
        i = (i + 1) & 0xff;		
        j = (j + S[i]) & 0xff;		
        S_SWAP(i, j);		
        *pos++ ^= S[(S[i] + S[j]) & 0xff];	
    }	
    return 0;
}                 
#endif


/**@brief Function for application main entry.
 */
int main(void)
{
    uint32_t err_code;
    bool erase_bonds;	    

	timers_init();
    ble_stack_init();
    device_manager_init(erase_bonds);
    gap_params_init();
    services_init();
    conn_params_init();
    Sys_Init();       
    advertising_init();
	//SendUart_data(wdat,4);
	
    m_ble_adv_status = BLE_ADV_STOP;
    if(Sys_EnableBlueTooth())
    {
		err_code = ble_advertising_start(BLE_ADV_MODE_FAST);
		APP_ERROR_CHECK(err_code);
		m_ble_adv_timer = 1000/64;
    }
	//Battdet_timer_start(1000);
	Carddet_timer_start(500);  
	SysBasetimer_Start(64);
#if (defined WATCHDOG_EN) && (WATCHDOG_EN == STD_TRUE)
	Sys_wdt_start();
#endif
#if (defined TOUCH_ENABLE) && (TOUCH_ENABLE == STD_TRUE)	
	TouchTimer64ms = 2000/64;
#endif


	//rc4_skip(Aes_KeyTable,16,0,Aes_KeyTable,16 );


    for (;;)
    {
		power_enter_sleepdet();
    }
}
