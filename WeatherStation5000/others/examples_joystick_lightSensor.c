/*****************************************************************************
*   This example is controlling the LEDs using the joystick
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
#include "gpio.h"
#include "i2c.h"
#include "ssp.h"
#include "adc.h"

#include "joystick.h"
#include "pca9532.h"
#include "acc.h"
#include "rotary.h"
#include "led7seg.h"
#include "oled.h"
#include "rgb.h"

//##################################### EXAMPLE_1 #####################################
//##################################### JOYSTICK  #####################################

static void drawOled(uint8_t joyState)
{
	static int wait = 0;
	static uint8_t currX = 48;
	static uint8_t currY = 32;
	static uint8_t lastX = 0;
	static uint8_t lastY = 0;

	if ((joyState & JOYSTICK_CENTER) != 0) {
		oled_clearScreen(OLED_COLOR_BLACK);
		return;
	}

	if (wait++ < 3)
		return;

	wait = 0;

	if ((joyState & JOYSTICK_UP) != 0 && currY > 0) {
		currY--;
	}

	if ((joyState & JOYSTICK_DOWN) != 0 && currY < OLED_DISPLAY_HEIGHT - 1) {
		currY++;
	}

	if ((joyState & JOYSTICK_RIGHT) != 0 && currX < OLED_DISPLAY_WIDTH - 1) {
		currX++;
	}

	if ((joyState & JOYSTICK_LEFT) != 0 && currX > 0) {
		currX--;
	}

	if (lastX != currX || lastY != currY) {
		oled_putPixel(currX, currY, OLED_COLOR_WHITE);
		lastX = currX;
		lastY = currY;
	}
}

#define P1_2_HIGH() (LPC_GPIO1->DATA |= (0x1<<2))
#define P1_2_LOW()  (LPC_GPIO1->DATA &= ~(0x1<<2))

int main(void) {

	uint8_t dir = 1;
	uint8_t wait = 0;

	uint8_t state = 0;

	uint32_t trim = 0;

	uint8_t btn1 = 0;

	GPIOInit();
	init_timer32(0, 10);

	UARTInit(115200);
	UARTSendString((uint8_t*)"Demo\r\n");

	I2CInit((uint32_t)I2CMASTER, 0);
	SSPInit();
	ADCInit(ADC_CLK);

	joystick_init();
	oled_init();

	while (1)
	{
		/* # */
		/* ############################################# */


		/* ####### Joystick and OLED  ###### */
		/* # */

		state = joystick_read();
		if (state != 0)
			drawOled(state);

		/* # */
		/* ############################################# */

		btn1 = GPIOGetValue(PORT0, 1);
	}
}


//##################################### EXAMPLE_2 #####################################
//##################################### JOYSTICK  #####################################



int main(void) {

	uint8_t dir = 0;
	uint32_t delay = 100;
	uint32_t cnt = 0;
	uint16_t ledOn = 0;
	uint16_t ledOff = 0;

	uint8_t joy = 0;

	GPIOInit();
	init_timer32(0, 10);

	UARTInit(115200);
	UARTSendString((uint8_t*)"LEDs (PCA9532)\r\n");

	I2CInit((uint32_t)I2CMASTER, 0);

	pca9532_init();
	joystick_init();

	while (1) {

		joy = joystick_read();

		if ((joy & JOYSTICK_CENTER) != 0) {
			continue;
		}

		if ((joy & JOYSTICK_DOWN) != 0) {
			if (delay < 200)
				delay += 10;
		}

		if ((joy & JOYSTICK_UP) != 0) {
			if (delay > 30)
				delay -= 10;
		}

		if ((joy & JOYSTICK_LEFT) != 0) {
			dir = 0;
		}

		if ((joy & JOYSTICK_RIGHT) != 0) {
			dir = 1;
		}


		if (cnt < 16)
			ledOn |= (1 << cnt);
		if (cnt > 15)
			ledOn &= ~(1 << (cnt - 16));

		if (cnt > 15)
			ledOff |= (1 << (cnt - 16));
		if (cnt < 16)
			ledOff &= ~(1 << cnt);

		pca9532_setLeds(ledOn, ledOff);

		if (dir) {
			if (cnt == 0)
				cnt = 31;
			else
				cnt--;

		}
		else {
			cnt++;
			if (cnt >= 32)
				cnt = 0;
		}

		delay32Ms(0, delay);
	}


}

//##################################### EXAMPLE_3 #####################################
//################################### LIGHT_SENSOR ####################################


/*
* NOTE: I2C must have been initialized before calling any functions in this
* file.
*/
/******************************************************************************
* Includes
*****************************************************************************/

#include "mcu_regs.h"
#include "type.h"
#include "i2c.h"
#include "light.h"

light_init(void)
light_enable(void)
light_read(void) // podobno odczytuje w luxach
light_shutdown(void)