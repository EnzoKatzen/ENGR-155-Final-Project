#pragma once

#include "stdlib.h"
#include "stdbool.h"

#include "stm32l4xx_hal.h"

extern I2C_HandleTypeDef __I2C1_HANDLE;

HAL_StatusTypeDef I2C1_init(void);
HAL_StatusTypeDef I2C1_teardown(void);