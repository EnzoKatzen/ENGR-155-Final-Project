#pragma once

#include "stdlib.h"
#include "stdbool.h"

#include "stm32l4xx_hal.h"

extern TIM_HandleTypeDef __TIM2_HANDLE; // master
extern TIM_HandleTypeDef __TIM1_HANDLE; // slave

HAL_StatusTypeDef TIM2_init(void);
HAL_StatusTypeDef TIM2_teardown(void);
HAL_StatusTypeDef TIM1_init(void);
HAL_StatusTypeDef TIM1_teardown(void);

HAL_StatusTypeDef timer_start(void);
HAL_StatusTypeDef timer_stop(void);
uint64_t timer_read(void);
uint64_t timer_delta(uint64_t begin, uint64_t end);