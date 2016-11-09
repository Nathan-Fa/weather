/*
 * pressure.h
 *
 *  Created on: Nov 9, 2016
 *      Author: embedded
 */

#ifndef PRESSURE_H_
#define PRESSURE_H_

#include "i2c.h"

void init_pressure();
long get_pressure();

#endif /* PRESSURE_H_ */
