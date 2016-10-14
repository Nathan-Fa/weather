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


#include "light.h"
#include "oled.h"
#include "temp.h"


static uint32_t msTicks = 0;
static uint8_t buf[10];

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

    GPIOInit();
    init_timer32(0, 10);

    UARTInit(115200);
    UARTSendString((uint8_t*)"WeatherStation5000\r\n");

    I2CInit( (uint32_t)I2CMASTER, 0 );
    SSPInit();
    ADCInit( ADC_CLK );

    oled_init();
    light_init();
    temp_init(&getTicks);


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
    light_setRange(LIGHT_RANGE_4000);

    oled_clearScreen(OLED_COLOR_WHITE);
    oled_putString(1,1,  (uint8_t*)"Temp   : ", OLED_COLOR_BLACK, OLED_COLOR_WHITE);
    oled_putString(1,9,  (uint8_t*)"Light  : ", OLED_COLOR_BLACK, OLED_COLOR_WHITE);

    while(1) {


        /* Temperature */
        t = temp_read();

        /* light */
        lux = light_read();

        /* output values to OLED display */
        intToString(t, buf, 10, 10);
        oled_fillRect((1+9*7),1, 80, 8, OLED_COLOR_WHITE);
        oled_putString((1+9*7),1, buf, OLED_COLOR_BLACK, OLED_COLOR_WHITE);

        intToString(lux, buf, 10, 10);
        oled_fillRect((1+9*6),9, 80, 16, OLED_COLOR_WHITE);
        oled_putString((1+9*6),9, buf, OLED_COLOR_BLACK, OLED_COLOR_WHITE);

        /* delay */
        delay32Ms(0, 200);
    }

}
