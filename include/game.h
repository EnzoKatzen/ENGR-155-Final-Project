#pragma once

#include "stdlib.h"
#include "stdbool.h"
#include "stdio.h"
#include "math.h"

#include "stm32l4xx_hal.h"
#include "periph/tim.h"
#include "periph/i2c.h"
#include "periph/spi.h"

#include "components/accel.h"
#include "modules/tilt.h"

void game_start(void);