#pragma once

#include "stdlib.h"
#include "stdbool.h"

#include "stm32l4xx_hal.h"

extern UART_HandleTypeDef __UART2_HANDLE;

HAL_StatusTypeDef UART2_init(void);
HAL_StatusTypeDef UART2_teardown(void);