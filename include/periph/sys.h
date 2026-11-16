#pragma once

#include "stdlib.h"
#include "stdbool.h"
#include "stdio.h"

#include "stm32l4xx_hal.h"
#include "periph/uart.h"

int _write(int file, char *ptr, int len);

HAL_StatusTypeDef sysclk_config(void);
void print_config(void);