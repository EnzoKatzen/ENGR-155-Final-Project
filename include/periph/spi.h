#pragma once

#include "stdlib.h"
#include "stdbool.h"

#include "stm32l4xx_hal.h"

extern SPI_HandleTypeDef __SPI1_HANDLE;

HAL_StatusTypeDef SPI1_init(void);
HAL_StatusTypeDef SPI1_teardown(void);