#pragma once

#include "stdlib.h"
#include "stdbool.h"

#include "stm32l4xx_hal.h"

extern ADC_HandleTypeDef __ADC1_HANDLE;

HAL_StatusTypeDef ADC1_init(void);
HAL_StatusTypeDef ADC1_teardown(void);
void ADC1_arm(void);