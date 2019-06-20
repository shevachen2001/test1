/**************************************************************************************************
 *                                            INCLUDES
 **************************************************************************************************/

#include "Tsmxx.h"
#include  "SYSTEM\Sys_GenFun.h"
#include  "I2C\I2c_Soft.h"
#include  "KEY\key.h"
#include  "KEY\TSMxx.h"
#include "Access\Access.h"
#include "Light\light.h"
#include  "KEY\Touch.h"

#include"PUBLIC\Std_Types.h"


extern Std_ReturnType power_out_sleep(void);
/****************************************************************************************************
**Function:
   void Tsmxx_Irq_Init(void)
**Author: lxw
**Description:
**Input: 
**Output: 
****************************************************************************************************/
void Tsmxx_Irq_Init(void)
{
	// ����BUTTON_1�ܽ�ΪDETECT�ź�������þ����Ҫ����CPU��GPIO���ѵı�Ҫ����  
	nrf_gpio_cfg_sense_input(TOUCH_INTT_PIN, NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_SENSE_LOW);	 
	NRF_GPIOTE->INTENSET = 1 << 31;
	NVIC_SetPriority(GPIOTE_IRQn, 3);
	NVIC_ClearPendingIRQ(GPIOTE_IRQn);
	NVIC_EnableIRQ(GPIOTE_IRQn);
}
/****************************************************************************************************
**Function:
   void Tsmxx_Irq_Disable(void)
**Author: lxw
**Description:
**Input: 
**Output: 
****************************************************************************************************/
void Tsmxx_Irq_Disable(void)
{
	nrf_gpio_cfg_sense_input(TOUCH_INTT_PIN, NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_SENSE_LOW);	 
	NVIC_DisableIRQ(GPIOTE_IRQn);
}

void Wiegand_Init(void) 
{
  nrf_gpio_cfg_output(WIEGAND_DATA0_PIN);
  nrf_gpio_cfg_output(WIEGAND_DATA1_PIN);
}

void WG_send_bit_1(void)
{
    //WG_DATA1 = 0;
    //----------------------��ʱ100us
    //delay_100us();
    //WG_DATA1 = 1;
    //-------------------------------��ʱһ����������
    //delay_1500us(); 
	WIEGAND_DATA1Low(); 
	Sys_GenFunDelayUs(100);
	WIEGAND_DATA1High(); 
    Sys_GenFunDelayUs(1500); 
}
void WG_send_bit_0(void)
{
    //WG_DATA0 = 0;	
    //----------------------��ʱ100us
    //delay_100us();
    //WG_DATA0 = 1;
    //-------------------------------��ʱһ����������
    //delay_1500us();    

	WIEGAND_DATA0Low(); 
	Sys_GenFunDelayUs(100);
	WIEGAND_DATA0High(); 
    Sys_GenFunDelayUs(1500);    
}

    unsigned char i;
    unsigned char check_temp; //Τ������żЧ���м��ݴ�
    unsigned char even; //Τ����ǰ12λżЧ��
    unsigned char odd; //Τ������12λ��Ч��
    unsigned char wiegand[3]; //Τ��������24λ

void Send_Weigand34(unsigned char *str)
{
    //| wiegand[0] | wiegand[1] | wiegand[2] |
    //| *str *(str + 1) | *(str + 2) *(str + 3)| *(str + 4) *(str + 5)|
 
 
    //--------------------------------�˿ڷ�����
	Wiegand_Init();

    //================================���鵽Τ������ת��
    wiegand[0] = wiegand[0]|((*str)<<4);//ԭ���ǰ�ÿ���ֽڵĵ�4λȡ��������������ֽڵ�ֵ
    wiegand[0] = wiegand[0]|(*(str+1)&0x0f);
    wiegand[1] = wiegand[1]|(*(str+2)<<4);
    wiegand[1] = wiegand[1]|(*(str+3)&0x0f);
    wiegand[2] = wiegand[2]|(*(str+4)<<4);
    wiegand[2] = wiegand[2]|(*(str+5)&0x0f);
 
    //--------------------------------����ǰ12λ1�ĸ����Ƿ�Ϊż����ΪżЧ����
    check_temp = wiegand[1]&0xf0;
    check_temp ^= wiegand[0];
    check_temp ^= check_temp>>4;
    check_temp ^= check_temp>>2;
    check_temp ^= check_temp>>1;
    even=!(check_temp&1);
 
    //--------------------------------�����12λ1�ĸ����Ƿ�Ϊż����Ϊ��Ч����
    check_temp = wiegand[1]&0x0f;
    check_temp ^= wiegand[2];
    check_temp ^= check_temp>>4;
    check_temp ^= check_temp>>2;
    check_temp ^= check_temp>>1;
    odd=check_temp&1;
 
    //================================�������ͣ��ö�ʱ����ʱ����ʱ
    //--------------------------------Τ�� ����˳�ʼ��
	WIEGAND_DATA0High(); 
	WIEGAND_DATA1High(); 
    //--------------------------------����żЧ��
    if(even)
    {
        WG_send_bit_1();
    }
    else
    {
        WG_send_bit_0();
    }
    //-------------------------------����24λ����
    for(i = 0;i<24;i++)
    {
        //---------------------------Τ�� ����˳�ʼ��
        if((wiegand[0])&0x80)
        {
            WG_send_bit_1();
        }
        else
        {
            WG_send_bit_0();
        }
        (*(long*)&wiegand[0]) <<= 1;
    }
    //==============================������Ч��λ
    if(odd)
    {
        WG_send_bit_1();
    }
    else
    {
        WG_send_bit_0();
    }
}
