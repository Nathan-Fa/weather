#include "mcu_regs.h"
#include "type.h"
#include "uart.h"
#include "stdio.h"
#include "timer32.h" // delay32Ms
#include "i2c.h"
#include "../include/pressure180.h"

s8 BMP180_I2C_bus_write(u8 dev_addr, u8 reg_addr, u8 *reg_data, u8 cnt)
{
	u8 stringpos=0;
	u8 array[8] = { 0 };
	array[0] = reg_addr;

	while (stringpos < cnt)
	{
		array[stringpos - 1] = *(reg_data + stringpos);
		stringpos++;
	}

	I2CWrite(0xEE, array, cnt-1);


	return BMP180_INIT_VALUE;
}

//------------------------------------------------------------------------
s8 BMP180_I2C_bus_read(u8 dev_addr, u8 reg_addr, u8 *reg_data, u8 cnt)
{
	u8 stringpos=0;
	uint8_t byte = 0;

	unsigned char buf[1] = { reg_addr };

	while (stringpos < cnt)
	{
		u8 *addr = reg_addr + stringpos;

		I2CWrite(0xEE, addr, 1);
		I2CRead(0xEF, &byte, 1);

		*(reg_data + stringpos) = byte;
		stringpos++;
	}

	return BMP180_INIT_VALUE;
}

//------------------------------------------------------------------------
void BMP180_delay_msek(u32 msek)
{
	delay32Ms(0, msek);
}

//------------------------------------------------------------------------


// Struktura do operacji na BMP180
struct bmp180_t bmp180;

s32 BMP180Init()
{
	s32 com_rslt = E_BMP_COMM_RES;
	u16 v_uncomp_temp_u16 =  BMP180_INIT_VALUE;
	u32 v_uncomp_press_u32 = BMP180_INIT_VALUE;


	// Przypisz odpowiednie funkcje
	bmp180.bus_write = BMP180_I2C_bus_write;
	bmp180.bus_read =  BMP180_I2C_bus_read;
	bmp180.dev_addr =  BMP180_I2C_ADDR;
	bmp180.delay_msec= BMP180_delay_msek;


	com_rslt =  bmp180_init(&bmp180);

	com_rslt += bmp180_get_calib_param();


	// Czytaj nieprzetworzone dane.
	v_uncomp_temp_u16 =  bmp180_get_uncomp_temperature();
	v_uncomp_press_u32 = bmp180_get_uncomp_pressure();

	//if (v_uncomp_press_u32 == 0xF6F6 || v_uncomp_temp_u16 == 0xF6F6)
	//	return -1;

	// Czytaj prawdziwe dane
	com_rslt += bmp180_get_temperature(v_uncomp_temp_u16);
	com_rslt += bmp180_get_pressure(v_uncomp_press_u32);


	return com_rslt;
}
