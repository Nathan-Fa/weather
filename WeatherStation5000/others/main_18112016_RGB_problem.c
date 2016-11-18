/*****************************************************************************
 *   Peripherals such as temp sensor, light sensor, accelerometer,
 *   and trim potentiometer are monitored and values are written to
 *   the OLED display.
 *
 *   Copyright(C) 2009, Embedded Artists AB
 *   All rights reserved.
 *
 ******************************************************************************/


#include "mcu_regs.h"
#include "type.h"
#include "uart.h"
#include "stdio.h"
#include "timer32.h"
#include "i2c.h"
#include "gpio.h"
#include "ssp.h"
#include "adc.h"
#include "rgb.h"


#include "light.h"
#include "oled.h"
#include "temp.h"
#include "joystick.h"


static uint32_t msTicks = 0;
static uint8_t buf[10];
static const uint8_t MAX_PAGE = 1;
static const uint32_t TOP_LEFT = 28;

static void intToString(int value, uint8_t* pBuf, uint32_t len, uint32_t base)
{
    static const char* pAscii = "0123456789abcdefghijklmnopqrstuvwxyz";
    int pos = 0;
    int tmpValue = value;

    // the buffer must not be null and at least have a length of 2 to handle one
    // digit and null-terminator
    if (pBuf == NULL || len < 2)
    {
        return;
    }

    // a valid base cannot be less than 2 or larger than 36
    // a base value of 2 means binary representation. A value of 1 would mean only zeros
    // a base larger than 36 can only be used if a larger alphabet were used.
    if (base < 2 || base > 36)
    {
        return;
    }

    // negative value
    if (value < 0)
    {
        tmpValue = -tmpValue;
        value    = -value;
        pBuf[pos++] = '-';
    }

    // calculate the required length of the buffer
    do {
        pos++;
        tmpValue /= base;
    } while(tmpValue > 0);


    if (pos > len)
    {
        // the len parameter is invalid.
        return;
    }

    pBuf[pos] = '\0';

    do {
        pBuf[--pos] = pAscii[value % base];
        value /= base;
    } while(value > 0);

    return;

}

void SysTick_Handler(void) {
    msTicks++;
}

static uint32_t getTicks(void)
{
    return msTicks;
}


int main (void)
{
    int32_t t = 0;
    uint32_t lux = 0;
    uint8_t joy = 0;
    int8_t current_page = 0;
    uint8_t buf2[1];

    GPIOInit();
    GPIOSetDir(PORT0, 1, 0);

    init_timer32(0, 10);

    UARTInit(115200);
    UARTSendString((uint8_t*)"WeatherStation5000\r\n");

    I2CInit( (uint32_t)I2CMASTER, 0 );
    SSPInit();
    ADCInit( ADC_CLK );


    light_init();
    temp_init(&getTicks);
    joystick_init();
    rgb_init();
    rgb_setLeds(RGB_RED);
    oled_init();

    /* setup sys Tick. Elapsed time is e.g. needed by temperature sensor */
    SysTick_Config(SystemCoreClock / 1000);
    if ( !(SysTick->CTRL & (1<<SysTick_CTRL_CLKSOURCE_Msk)) )
    {
      /* When external reference clock is used(CLKSOURCE in
      Systick Control and register bit 2 is set to 0), the
      SYSTICKCLKDIV must be a non-zero value and 2.5 times
      faster than the reference clock.
      When core clock, or system AHB clock, is used(CLKSOURCE
      in Systick Control and register bit 2 is set to 1), the
      SYSTICKCLKDIV has no effect to the SYSTICK frequency. See
      more on Systick clock and status register in Cortex-M3
      technical Reference Manual. */
      LPC_SYSCON->SYSTICKCLKDIV = 0x08;
    }

    light_enable();
    light_setRange(LIGHT_RANGE_16000);

    oled_clearScreen(OLED_COLOR_BLACK);

	//oled_putString(1,TOP_LEFT,  (uint8_t*)"Temp   : ",OLED_COLOR_WHITE , OLED_COLOR_BLACK);

	LPC_IOCON->JTAG_nTRST_PIO1_2 = (LPC_IOCON->JTAG_nTRST_PIO1_2 & ~0x7) | 0x01;
	LPC_IOCON->PIO0_1 &= ~0x7;
	LPC_IOCON->PIO0_1 = LPC_IOCON->PIO0_1 & (~0x7); // enable button

    while(1)
    {
    	//joystick read
    	joy = joystick_read();
    	uint8_t changed = 0;
		if ((joy & JOYSTICK_LEFT) != 0)
		{
			current_page--;
			if(current_page == -1)
				current_page = MAX_PAGE;
			changed = 1;

		}
		else if ((joy & JOYSTICK_RIGHT) != 0 || GPIOGetValue(PORT0, 1) == 0) //0 means true
		{
			current_page++;
			if(current_page == MAX_PAGE+1)
				current_page = 0;
			changed = 1;

		}

		switch(current_page)
			{
				case 0: /* Temperature */
				{

					t = temp_read();
					intToString(t, buf, 10, 10);
					buf2[0] = buf[2];
					buf[2] = '\0';
					if(changed == 1) //refresh label
						oled_putString(1,TOP_LEFT,  (uint8_t*)"Temp   : ",OLED_COLOR_WHITE ,OLED_COLOR_BLACK );

					oled_fillRect((1+9*7),TOP_LEFT, 90, TOP_LEFT+8, OLED_COLOR_BLACK);
					oled_putString((1+9*7),TOP_LEFT, buf, OLED_COLOR_WHITE ,OLED_COLOR_BLACK);
					oled_putPixel((1+9*7) + 14,TOP_LEFT + 6,OLED_COLOR_WHITE);
					oled_putString((1+9*7) + 16,TOP_LEFT, buf2, OLED_COLOR_WHITE ,OLED_COLOR_BLACK);

					delay32Ms(0, 100);
					rgb_setLeds(RGB_GREEN);

					break;
				}


				case 1: /* Light */
				{

					rgb_setLeds(RGB_RED);
					delay32Ms(0, 100);

					lux = light_read();
					intToString(lux, buf, 10, 10);

					if(changed == 1) //refresh label
						oled_putString(1,TOP_LEFT,  (uint8_t*)"Light  : ", OLED_COLOR_WHITE,OLED_COLOR_BLACK );

					oled_fillRect((1+9*7),TOP_LEFT,90, TOP_LEFT+8, OLED_COLOR_BLACK);
					oled_putString((1+9*7),TOP_LEFT, buf,OLED_COLOR_WHITE ,OLED_COLOR_BLACK );



					break;
				}
			}

        /* delay */
        delay32Ms(0, 100);
    }

}
