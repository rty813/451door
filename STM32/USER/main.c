#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "led.h"
#include "timer.h"
#include "remote.h"
#include <math.h>
#include <string.h>
#include "joypad.h" 

int main(void)
{	
	uart_init(115200);
	delay_init();	    //延时函数初始化	  
	LED_Init();		  	//初始化与LED连接的硬件接口
	
	printf("at+sleep\r\n");
	while (1)
	{
		delay_ms(10);
		while (!(USART_RX_STA&0x8000))
		{
			continue;
		}
		USART_RX_STA = 0;
		if ((USART_RX_BUF[0] != '4')
			||(USART_RX_BUF[1] != '5')
			||(USART_RX_BUF[2] != '1'))
		{
			continue;
		}
		LED = !LED;
		DOOR = 1;
		USART_RX_STA = 0;
		delay_ms(1000);
		delay_ms(1000);
		DOOR = 0;
	}

	
}
