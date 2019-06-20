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

#include "bsp.h"
#include <stddef.h>
#include <stdio.h>
#include "nordic_common.h"
#include "nrf.h"
#include "nrf_gpio.h"
#include "nrf_error.h"

#ifdef BSP_UART_SUPPORT
#include "nrf_log.h"
#endif // BSP_UART_SUPPORT

#ifndef BSP_SIMPLE
#include "app_timer.h"
#include "app_button.h"
#endif // BSP_SIMPLE

#define ADVERTISING_LED_ON_INTERVAL            200
#define ADVERTISING_LED_OFF_INTERVAL           1800

#define ADVERTISING_DIRECTED_LED_ON_INTERVAL   200
#define ADVERTISING_DIRECTED_LED_OFF_INTERVAL  200

#define ADVERTISING_WHITELIST_LED_ON_INTERVAL  200
#define ADVERTISING_WHITELIST_LED_OFF_INTERVAL 800

#define ADVERTISING_SLOW_LED_ON_INTERVAL       400
#define ADVERTISING_SLOW_LED_OFF_INTERVAL      4000

#define BONDING_INTERVAL                       100

#define SENT_OK_INTERVAL                       100
#define SEND_ERROR_INTERVAL                    500

#define RCV_OK_INTERVAL                        100
#define RCV_ERROR_INTERVAL                     500

#define ALERT_INTERVAL                         200

#if LEDS_NUMBER > 0 && !(defined BSP_SIMPLE)
static bsp_indication_t m_stable_state        = BSP_INDICATE_IDLE;
static uint32_t         m_app_ticks_per_100ms = 0;
static uint32_t         m_indication_type     = 0;
static uint32_t         m_alert_mask          = 0;
APP_TIMER_DEF(m_leds_timer_id);
APP_TIMER_DEF(m_alert_timer_id);
#endif // LEDS_NUMBER > 0 && !(defined BSP_SIMPLE)

#if BUTTONS_NUMBER > 0
#ifndef BSP_SIMPLE
static bsp_event_callback_t   m_registered_callback         = NULL;
static bsp_button_event_cfg_t m_events_list[BUTTONS_NUMBER] = {{BSP_EVENT_NOTHING, BSP_EVENT_NOTHING}};
APP_TIMER_DEF(m_button_timer_id);
static void bsp_button_event_handler(uint8_t pin_no, uint8_t button_action);
#endif // BSP_SIMPLE

static const uint32_t m_buttons_list[BUTTONS_NUMBER] = BUTTONS_LIST;

#ifndef BSP_SIMPLE
static const app_button_cfg_t app_buttons[BUTTONS_NUMBER] =
{
    #ifdef BSP_BUTTON_0
    {BSP_BUTTON_0, false, BUTTON_PULL, bsp_button_event_handler},
    #endif // BUTTON_0

    #ifdef BSP_BUTTON_1
    {BSP_BUTTON_1, false, BUTTON_PULL, bsp_button_event_handler},
    #endif // BUTTON_1

    #ifdef BSP_BUTTON_2
    {BSP_BUTTON_2, false, BUTTON_PULL, bsp_button_event_handler},
    #endif // BUTTON_2

    #ifdef BSP_BUTTON_3
    {BSP_BUTTON_3, false, BUTTON_PULL, bsp_button_event_handler},
    #endif // BUTTON_3

};
#endif // BSP_SIMPLE
#endif // BUTTONS_NUMBER > 0

#define BSP_MS_TO_TICK(MS) (m_app_ticks_per_100ms * (MS / 100))

#ifdef BSP_LED_2_MASK
#define ALERT_LED_MASK BSP_LED_2_MASK
#else
#define ALERT_LED_MASK BSP_LED_1_MASK
#endif // LED_2_MASK

uint32_t bsp_buttons_state_get(uint32_t * p_buttons_state)
{
    uint32_t result = NRF_SUCCESS;

    *p_buttons_state = 0;
#if BUTTONS_NUMBER > 0
    uint32_t buttons = ~NRF_GPIO->IN;
    uint32_t cnt;

    for (cnt = 0; cnt < BUTTONS_NUMBER; cnt++)
    {
        if (buttons & (1 << m_buttons_list[cnt]))
        {
            *p_buttons_state |= 1 << cnt;
        }
    }
#endif // BUTTONS_NUMBER > 0

    return result;
}


uint32_t bsp_button_is_pressed(uint32_t button, bool * p_state)
{
#if BUTTONS_NUMBER > 0
    if(button < BUTTONS_NUMBER)
    {
        uint32_t buttons = ~NRF_GPIO->IN;
        *p_state = (buttons & (1 << m_buttons_list[button])) ? true : false;
    }
    else
    {
        *p_state = false;
    }
#else
    *p_state = false;
#endif // BUTTONS_NUMBER > 0
    return NRF_SUCCESS;
}


#if (BUTTONS_NUMBER > 0) && !(defined BSP_SIMPLE)
/**@brief Function for handling button events.
 *
 * @param[in]   pin_no          The pin number of the button pressed.
 * @param[in]   button_action   Action button.
 */
extern Std_ReturnType power_out_sleep(void);
static void bsp_button_event_handler(uint8_t pin_no, uint8_t button_action)
{
#if 0
    bsp_event_t        event  = BSP_EVENT_NOTHING;
    uint32_t           button = 0;
    uint32_t           err_code;
    static uint8_t     current_long_push_pin_no;              /**< Pin number of a currently pushed button, that could become a long push if held long enough. */
    static bsp_event_t release_event_at_push[BUTTONS_NUMBER]; /**< Array of what the release event of each button was last time it was pushed, so that no release event is sent if the event was bound after the push of the button. */

    while ((button < BUTTONS_NUMBER) && (m_buttons_list[button] != pin_no))
    {
        button++;
    }

    if (button < BUTTONS_NUMBER)
    {
        switch(button_action)
        {
            case APP_BUTTON_PUSH:
                event = m_events_list[button].push_event;
                if (m_events_list[button].long_push_event != BSP_EVENT_NOTHING)
                {
                    err_code = app_timer_start(m_button_timer_id, BSP_MS_TO_TICK(BSP_LONG_PUSH_TIMEOUT_MS), (void*)&current_long_push_pin_no);
                    if (err_code == NRF_SUCCESS)
                    {
                        current_long_push_pin_no = pin_no;
                    }
                }
                release_event_at_push[button] = m_events_list[button].release_event;
                break;
            case APP_BUTTON_RELEASE:
                (void)app_timer_stop(m_button_timer_id);
                if (release_event_at_push[button] == m_events_list[button].release_event)
                {
                    event = m_events_list[button].release_event;
                }
                break;
            case BSP_BUTTON_ACTION_LONG_PUSH:
                event = m_events_list[button].long_push_event;
        }
    }

    if ((event != BSP_EVENT_NOTHING) && (m_registered_callback != NULL))
    {
        m_registered_callback(event);
    }

	#endif
}

/**@brief Handle events from button timer.
 *
 * @param[in]   p_context   parameter registered in timer start function.
 */
static void button_timer_handler(void * p_context)
{
    bsp_button_event_handler(*(uint8_t *)p_context, BSP_BUTTON_ACTION_LONG_PUSH);
}


#endif // (BUTTONS_NUMBER > 0) && !(defined BSP_SIMPLE)



/**@brief Configure indicators to required state.
 */
uint32_t bsp_indication_set(bsp_indication_t indicate)
{
    uint32_t err_code = NRF_SUCCESS;

#if LEDS_NUMBER > 0 && !(defined BSP_SIMPLE)

    if (m_indication_type & BSP_INIT_LED)
    {
        err_code = bsp_led_indication(indicate);
    }

#endif // LEDS_NUMBER > 0 && !(defined BSP_SIMPLE)
    return err_code;
}


uint32_t bsp_indication_text_set(bsp_indication_t indicate, char const * p_text)
{
    uint32_t err_code = bsp_indication_set(indicate);

#ifdef BSP_UART_SUPPORT
    NRF_LOG_PRINTF("%s", p_text);
#endif // BSP_UART_SUPPORT

    return err_code;
}


uint32_t bsp_init(uint32_t type, uint32_t ticks_per_100ms, bsp_event_callback_t callback)
{
    uint32_t err_code = NRF_SUCCESS;

#if LEDS_NUMBER > 0 && !(defined BSP_SIMPLE)
    m_app_ticks_per_100ms = ticks_per_100ms;
    m_indication_type     = type;
#else
    UNUSED_VARIABLE(ticks_per_100ms);
#endif // LEDS_NUMBER > 0 && !(defined BSP_SIMPLE)

#if (BUTTONS_NUMBER > 0) && !(defined BSP_SIMPLE)
    m_registered_callback = callback;

    // BSP will support buttons and generate events
    if (type & BSP_INIT_BUTTONS)
    {
        uint32_t num;

        for (num = 0; ((num < BUTTONS_NUMBER) && (err_code == NRF_SUCCESS)); num++)
        {
            err_code = bsp_event_to_button_action_assign(num, BSP_BUTTON_ACTION_PUSH, BSP_EVENT_DEFAULT);
        }

        if (err_code == NRF_SUCCESS)
        {
            err_code = app_button_init((app_button_cfg_t *)app_buttons,
                                       BUTTONS_NUMBER,
                                       ticks_per_100ms / 2);
        }

        if (err_code == NRF_SUCCESS)
        {
            err_code = app_button_enable();
        }

 /*       if (err_code == NRF_SUCCESS)
        {
            err_code = app_timer_create(&m_button_timer_id,
                                        APP_TIMER_MODE_SINGLE_SHOT,
                                        button_timer_handler);
        }*/
    }
#elif (BUTTONS_NUMBER > 0) && (defined BSP_SIMPLE)

    if (type & BSP_INIT_BUTTONS)
    {
        uint32_t cnt;
        uint32_t buttons[] = BUTTONS_LIST;

        for (cnt = 0; cnt < BUTTONS_NUMBER; cnt++)
        {
            nrf_gpio_cfg_input(buttons[cnt], BUTTON_PULL);
        }
    }
#endif // (BUTTONS_NUMBER > 0) && !(defined BSP_SIMPLE)


    return err_code;
}


#ifndef BSP_SIMPLE
/**@brief Assign specific event to button.
 */
uint32_t bsp_event_to_button_action_assign(uint32_t button, bsp_button_action_t action, bsp_event_t event)
{
    uint32_t err_code = NRF_SUCCESS;

#if BUTTONS_NUMBER > 0
    if (button < BUTTONS_NUMBER)
    {
        if (event == BSP_EVENT_DEFAULT)
        {
            // Setting default action: BSP_EVENT_KEY_x for PUSH actions, BSP_EVENT_NOTHING for RELEASE and LONG_PUSH actions.
            event = (action == BSP_BUTTON_ACTION_PUSH) ? (bsp_event_t)(BSP_EVENT_KEY_0 + button) : BSP_EVENT_NOTHING;
        }
        switch (action)
        {
            case BSP_BUTTON_ACTION_PUSH:
                m_events_list[button].push_event = event;
                break;
            case BSP_BUTTON_ACTION_LONG_PUSH:
                m_events_list[button].long_push_event = event;
                break;
            case BSP_BUTTON_ACTION_RELEASE:
                m_events_list[button].release_event = event;
                break;
            default:
                err_code = NRF_ERROR_INVALID_PARAM;
                break;
        }
    }
    else
    {
        err_code = NRF_ERROR_INVALID_PARAM;
    }
#else
    err_code = NRF_ERROR_INVALID_PARAM;
#endif // BUTTONS_NUMBER > 0

    return err_code;
}

#endif // BSP_SIMPLE


uint32_t bsp_buttons_enable()
{
#if (BUTTONS_NUMBER > 0) && !defined(BSP_SIMPLE)
    return app_button_enable();
#else
    return NRF_ERROR_NOT_SUPPORTED;
#endif
}

uint32_t bsp_buttons_disable()
{
#if (BUTTONS_NUMBER > 0) && !defined(BSP_SIMPLE)
    return app_button_disable();
#else
    return NRF_ERROR_NOT_SUPPORTED;
#endif
}

uint32_t bsp_wakeup_buttons_set(uint32_t wakeup_buttons)
{
#if (BUTTONS_NUMBER > 0) && !defined(BSP_SIMPLE)
    for (uint32_t i = 0; i < BUTTONS_NUMBER; i++)
    {
        uint32_t new_cnf = NRF_GPIO->PIN_CNF[m_buttons_list[i]];
        uint32_t new_sense = ((1 << i) & wakeup_buttons) ? GPIO_PIN_CNF_SENSE_Low : GPIO_PIN_CNF_SENSE_Disabled;
        new_cnf &= ~GPIO_PIN_CNF_SENSE_Msk;
        new_cnf |= (new_sense << GPIO_PIN_CNF_SENSE_Pos);
        NRF_GPIO->PIN_CNF[m_buttons_list[i]] = new_cnf;
    }
    return NRF_SUCCESS;
#else
    return NRF_ERROR_NOT_SUPPORTED;
#endif
}
