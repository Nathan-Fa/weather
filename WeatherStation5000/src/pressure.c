#include "mcu_regs.h"
#include "type.h"
#include "uart.h"
#include "stdio.h"
#include "timer32.h"
#include "../include/pressure.h"

#define BMP180_ADDRESS 0x77  // I2C address of BMP085

const unsigned char OSS = 0;  // Oversampling Setting

//prototypes
char bmp085Read(unsigned char address);
int bmp085ReadInt(unsigned char address);
void bmp085Calibration();
unsigned int bmp085ReadUT();
unsigned long bmp085ReadUP();
short bmp085GetTemperature(unsigned int ut);
long bmp085GetPressure(unsigned long up);

// Calibration values
short ac1;
short ac2;
short ac3;
unsigned short ac4;
unsigned short ac5;
unsigned short ac6;
short b1;
short b2;
short mb;
short mc;
short md;

// b5 is calculated in bmp085GetTemperature(...), this variable is also used in bmp085GetPressure(...)
// so ...Temperature(...) must be called before ...Pressure(...).
long b5;

short temperature;
long pressure;

// Use these for altitude conversions
const float p0 = 101325;     // Pressure at sea level (Pa)
float altitude;

uint8_t init_pressure()
{
	  ac1 = bmp085ReadInt(0xAA);
	  ac2 = bmp085ReadInt(0xAC);
	  ac3 = bmp085ReadInt(0xAE);
	  ac4 = bmp085ReadInt(0xB0);
	  ac5 = bmp085ReadInt(0xB2);
	  ac6 = bmp085ReadInt(0xB4);
	  b1 = bmp085ReadInt(0xB6);
	  b2 = bmp085ReadInt(0xB8);
	  mb = bmp085ReadInt(0xBA);
	  mc = bmp085ReadInt(0xBC);
	  md = bmp085ReadInt(0xBE);

	  return (ac1 == ac2 && ac2 == ac3) ? 0 : 1;
}

long get_pressure()
{
	bmp085GetTemperature(bmp085ReadUT());
	return bmp085GetPressure(bmp085ReadUP());
}

// Read 1 byte from the BMP085 at 'address'
char bmp085Read(unsigned char address)
{
  unsigned char buf[1];
  unsigned char addr[1];
  addr[0] = address;
  I2CWrite(0xEE,addr,1);
  I2CRead(0xEF,buf,1);
  //Wire.beginTransmission(BMP180_ADDRESS);
  //Wire.write(address);
  //Wire.endTransmission();

  return buf[0];
}

// Read 2 bytes from the BMP085
// First byte will be from 'address'
// Second byte will be from 'address'+1
int bmp085ReadInt(unsigned char address)
{
  //Wire.beginTransmission(BMP180_ADDRESS);
  //Wire.write(address);
  //Wire.endTransmission();

  //Wire.requestFrom(BMP180_ADDRESS, 2);
  //while(Wire.available()<2)
  //  ;
  //msb = Wire.read();
  //lsb = Wire.read();

  unsigned char buf[2];
  unsigned char addr[1];
  addr[0] = address;
  I2CWrite(0xEE,addr,1);
  I2CRead(0xEF,buf,2);


  return (int) buf[0]<<8 | buf[1];
}


// Read the uncompensated temperature value
unsigned int bmp085ReadUT()
{
  unsigned int ut;

  // Write 0x2E into Register 0xF4
  // This requests a temperature reading
  //We have to write [0] - address [1] - value for addr [2] - value for addr+1
  unsigned char addr[2];
  addr[0] = 0xF4;
  addr[1] = 0x2E;
  I2CWrite(0xEE,addr,2);

  // Wait at least 4.5ms
  delay32Ms(0, 5);

  // Read two bytes from registers 0xF6 and 0xF7
  ut = bmp085ReadInt(0xF6);
  return ut;
}

// Read the uncompensated pressure value
unsigned long bmp085ReadUP()
{
  unsigned char msb, lsb, xlsb;
  unsigned long up = 0;

  // Write 0x34+(OSS<<6) into register 0xF4
  // Request a pressure reading w/ oversampling setting
  unsigned char addr[2];
  addr[0] = 0xF4;
  addr[1] = 0x34 + (OSS<<6);

  // Wait for conversion, delay time dependent on OSS
  delay32Ms(0,2 + (3<<OSS));

  // Read register 0xF6 (MSB), 0xF7 (LSB), and 0xF8 (XLSB)
  unsigned char buf[3];
  addr[0] = 0xF6;
  I2CWrite(0xEE,addr,1);
  I2CRead(0xEF,buf,3);

  msb = buf[0];
  lsb = buf[1];
  xlsb = buf[2];

  up = (((unsigned long) msb << 16) | ((unsigned long) lsb << 8) | (unsigned long) xlsb) >> (8-OSS);

  return up;
}


// Calculate temperature given ut.
// Value returned will be in units of 0.1 deg C
short bmp085GetTemperature(unsigned int ut)
{
  long x1, x2;

  x1 = (((long)ut - (long)ac6)*(long)ac5) >> 15;
  x2 = ((long)mc << 11)/(x1 + md);
  b5 = x1 + x2;

  return ((b5 + 8)>>4);
}

// Calculate pressure given up
// calibration values must be known
// b5 is also required so bmp085GetTemperature(...) must be called first.
// Value returned will be pressure in units of Pa.
long bmp085GetPressure(unsigned long up)
{
  long x1, x2, x3, b3, b6, p;
  unsigned long b4, b7;

  b6 = b5 - 4000;
  // Calculate B3
  x1 = (b2 * (b6 * b6)>>12)>>11;
  x2 = (ac2 * b6)>>11;
  x3 = x1 + x2;
  b3 = (((((long)ac1)*4 + x3)<<OSS) + 2)>>2;

  // Calculate B4
  x1 = (ac3 * b6)>>13;
  x2 = (b1 * ((b6 * b6)>>12))>>16;
  x3 = ((x1 + x2) + 2)>>2;
  b4 = (ac4 * (unsigned long)(x3 + 32768))>>15;

  b7 = ((unsigned long)(up - b3) * (50000>>OSS));
  if (b7 < 0x80000000)
    p = (b7<<1)/b4;
  else
    p = (b7/b4)<<1;

  x1 = (p>>8) * (p>>8);
  x1 = (x1 * 3038)>>16;
  x2 = (-7357 * p)>>16;
  p += (x1 + x2 + 3791)>>4;

  return p;
}
