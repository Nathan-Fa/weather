/*****************************************************************************
 *
 *   WeatherStation5000
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
#include "rotary.h"

#include "light.h"
#include "oled.h"
#include "temp.h"
#include "joystick.h"
#include "eeprom.h"
#include "../include/pressure.h"



static uint32_t msTicks = 0;
static uint8_t buf[10];
static const uint32_t TOP_LEFT = 28;

#define __min(a,b)	( (a <  b) ? a : b )
#define __max(a,b)	( (a >= b) ? a : b )

uint8_t delayTimeMs = 50;

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

void InitSysTick()
{
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

    // Clear the screen to black color


	LPC_IOCON->PIO0_1 = LPC_IOCON->PIO0_1 & (~0x7); // enable button
}


//------------------------------------------------------------------------
void RetrieveCachedData(uint8_t *pOutTemp, uint8_t *pOutLux, uint8_t *pOutPressure )
{
	char buf[32];
    memset(buf, 0, 32);

	int16_t len = eeprom_read(buf, 240, 32);

	if ( buf[0] == '7' && buf[1] == '6' && buf[2] == '5' )
	{
		 int32_t temp = 0;
		 uint32_t lux = 0;
		 int32_t pressure = 0;

		sscanf(buf, "765;%d;%d;%d;", &temp, &lux, &pressure);

		intToString(temp, pOutTemp, 8, 10);
		intToString(lux,  pOutLux,  8, 10);
		intToString(pressure, pOutPressure, 8, 10);
	}
	else
	{
		strcpy(pOutTemp,	 "empty");
		strcpy(pOutLux,  	 "empty");
		strcpy(pOutPressure, "empty");
	}
}
//------------------------------------------------------------------------
void SaveCachedData(uint8_t *pressure)
{
	int32_t temp = temp_read();
	uint32_t lux = light_read();

	/*
	uint8_t bufTemp[8];
	uint8_t bufLight[8];
	intToString(temp, bufTemp, 8, 10);
	intToString(lux, bufLight, 8, 10);
	*/

	char buffer[32];
	memset(buffer, 0, 32);

	sprintf(buffer, "765;%d;%d;%s;", temp, lux, pressure); //765 as control number

	int16_t len = eeprom_write(buffer, 240, 32);
	return;
}

//------------------------------------------------------------------------
int main (void)
{
	// Values from sensors
    int32_t temp = 0;
    uint32_t lux = 0;
    uint8_t joy = 0;

    uint8_t prevTemp[8];
    uint8_t prevLux[8];
    uint8_t prevPressure[8];


    int8_t current_page = 0;
    uint8_t buf2[1];
    uint8_t rotaryReadVal = ROTARY_WAIT;
    uint8_t max_page;
    uint8_t pressure[8];
    GPIOInit();
    GPIOSetDir(PORT0, 1, 0);
    init_timer32(0, 10);

    UARTInit(115200);
    UARTSendString((uint8_t*)"WeatherStation5000\r\n");


    I2CInit( (uint32_t)I2CMASTER, 0 );
    SSPInit();
    ADCInit( ADC_CLK );
    oled_init();
    light_init();
    temp_init(&getTicks);
    joystick_init();
    rgb_init();
    rotary_init();
    light_enable();
    InitSysTick();


    RetrieveCachedData(prevTemp, prevLux, prevPressure);
    light_setRange(LIGHT_RANGE_16000);

    oled_clearScreen(OLED_COLOR_BLACK);
	oled_putString(1,TOP_LEFT,  (uint8_t*)"Loading...",OLED_COLOR_WHITE , OLED_COLOR_BLACK);


	uint8_t isPressure = init_pressure();
    if(isPressure == 1)
    {
        oled_clearScreen(OLED_COLOR_BLACK);
    	oled_putString(1,TOP_LEFT,  (uint8_t*)"Calc. pressure...",OLED_COLOR_WHITE , OLED_COLOR_BLACK);
        intToString((int)get_pressure(), pressure, 8, 10);
        max_page = 2;
        rgb_setLeds(RGB_GREEN);
    }
    else
    {
    	max_page = 1;
    	rgb_setLeds(RGB_RED | RGB_GREEN);
    }

    SaveCachedData(pressure);

    oled_clearScreen(OLED_COLOR_BLACK);

    while(1)
    {
    	//joystick read
    	joy = joystick_read();
    	uint8_t changed = 0;
		if ((joy & JOYSTICK_LEFT) != 0)
		{
			current_page--;
			if(current_page == -1)
				current_page = max_page;
			changed = 1;
		}
		else if ((joy & JOYSTICK_RIGHT) != 0 || GPIOGetValue(PORT0, 1) == 0) //0 means true
		{
			current_page++;
			if(current_page == max_page+1)
				current_page = 0;
			changed = 1;
		}


		// Sprawdz stan rotacyjnego przelacznika kwadraturowego
		rotaryReadVal = rotary_read();
		switch (rotaryReadVal)
		{
		case ROTARY_RIGHT:
		{
			delayTimeMs -= 50;
			if(delayTimeMs < 1)
				delayTimeMs = 1;
			//delayTimeMs = __max(1, delayTimeMs);
			break;
		}

		case ROTARY_LEFT:
		{
			delayTimeMs += 50;
			if(delayTimeMs > 500)
				delayTimeMs = 500;
			//delayTimeMs = __min(200, delayTimeMs);
			break;
		}

		default:
			break;
		}


		switch(current_page)
			{
				case 0:
				{
					temp = temp_read();
					intToString(temp, buf, 10, 10);
					buf2[0] = buf[2];
					buf2[1] = '\0';
					buf[2] = '\0';
					if(changed == 1) //refresh label
					{
					    oled_clearScreen(OLED_COLOR_BLACK);
						oled_putString(1,TOP_LEFT,  (uint8_t*)"Temp   : ",OLED_COLOR_WHITE ,OLED_COLOR_BLACK );
					}

					oled_fillRect((1+9*7),TOP_LEFT, 90, TOP_LEFT+8, OLED_COLOR_BLACK);
					oled_putString((1+9*7),TOP_LEFT, buf, OLED_COLOR_WHITE ,OLED_COLOR_BLACK);
					oled_putPixel((1+9*7) + 14,TOP_LEFT + 6,OLED_COLOR_WHITE);
					oled_putString((1+9*7) + 16,TOP_LEFT, buf2, OLED_COLOR_WHITE ,OLED_COLOR_BLACK);

					oled_fillRect((1+9*6),TOP_LEFT+8,90, TOP_LEFT+16, OLED_COLOR_BLACK);
					oled_putString((1+9*6),TOP_LEFT+8, prevTemp,OLED_COLOR_WHITE ,OLED_COLOR_BLACK );
					break;
				}


				case 1:
				{
					lux = light_read();
					intToString(lux, buf, 10, 10);

					if(changed == 1) //refresh label
					{
						oled_clearScreen(OLED_COLOR_BLACK);
						oled_putString(1,TOP_LEFT,  (uint8_t*)"Light  : ", OLED_COLOR_WHITE,OLED_COLOR_BLACK );
					}

					oled_fillRect((1+9*7),TOP_LEFT,90, TOP_LEFT+8, OLED_COLOR_BLACK);
					oled_putString((1+9*7),TOP_LEFT, buf,OLED_COLOR_WHITE ,OLED_COLOR_BLACK );

					oled_fillRect((1+9*6),TOP_LEFT+8,90, TOP_LEFT+16, OLED_COLOR_BLACK);
					oled_putString((1+9*6),TOP_LEFT+8, prevLux,OLED_COLOR_WHITE ,OLED_COLOR_BLACK );
					break;
				}
				case 2:
				{
					if(changed == 1)
					{
						oled_clearScreen(OLED_COLOR_BLACK);
						oled_putString(1,TOP_LEFT,  (uint8_t*)"Press:", OLED_COLOR_WHITE,OLED_COLOR_BLACK );
						oled_fillRect((1+9*5),TOP_LEFT,90, TOP_LEFT+8, OLED_COLOR_BLACK);
						oled_putString((1+9*5),TOP_LEFT, pressure,OLED_COLOR_WHITE ,OLED_COLOR_BLACK );

						oled_fillRect((1+9*5),TOP_LEFT+8,90, TOP_LEFT+16, OLED_COLOR_BLACK);
						oled_putString((1+9*5),TOP_LEFT +8, prevPressure,OLED_COLOR_WHITE ,OLED_COLOR_BLACK );
					}
					break;
				}
			}


        /* delay */
        delay32Ms(0, delayTimeMs);
    }

}



