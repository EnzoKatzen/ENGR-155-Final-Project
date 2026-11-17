#pragma once

#include "stdlib.h"
#include "stdbool.h"

#include "stm32l4xx_hal.h"

extern SPI_HandleTypeDef __SPI1_HANDLE;
extern volatile uint32_t SPI1_data;
extern volatile bool SPI1_data_arrived;

HAL_StatusTypeDef SPI1_init(void);
HAL_StatusTypeDef SPI1_teardown(void);
HAL_StatusTypeDef SPI1_arm(void);