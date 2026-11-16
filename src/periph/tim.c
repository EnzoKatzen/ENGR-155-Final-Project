#include "periph/tim.h"

TIM_HandleTypeDef __TIM2_HANDLE = {};
TIM_HandleTypeDef __TIM1_HANDLE = {};

HAL_StatusTypeDef TIM2_init(void) {
  TIM_HandleTypeDef handle = {
    .Instance = TIM2,
    .Init = {
      .Prescaler = 79, // divide 80 MHz clock by 80 for a 1 MHz clock, 1us period
      .CounterMode = TIM_COUNTERMODE_UP,
      .Period = 0xFFFFFFFFu,
      .ClockDivision = TIM_CLOCKDIVISION_DIV1
    }
  };
  if (HAL_TIM_Base_Init(&handle) != HAL_OK) {
    return HAL_ERROR;
  }

  TIM_MasterConfigTypeDef master_config = {
    .MasterOutputTrigger = TIM_TRGO_UPDATE,
    .MasterSlaveMode = TIM_MASTERSLAVEMODE_ENABLE
  };
  if (HAL_TIMEx_MasterConfigSynchronization(&handle, &master_config) != HAL_OK) {
    return HAL_ERROR;
  }

  __TIM2_HANDLE = handle;
  return HAL_OK;
}

HAL_StatusTypeDef TIM2_teardown(void) {
  if (HAL_TIM_Base_DeInit(&__TIM2_HANDLE) != HAL_OK) {
    return HAL_ERROR;
  }

  return HAL_OK;
}

HAL_StatusTypeDef TIM1_init(void) {
  TIM_HandleTypeDef handle = {
    .Instance = TIM1,
    .Init = {
      .Prescaler = 0,
      .CounterMode = TIM_COUNTERMODE_UP,
      .Period = 0xFFFFu,
      .ClockDivision = TIM_CLOCKDIVISION_DIV1
    }
  };
  if (HAL_TIM_Base_Init(&handle) != HAL_OK) {
    return HAL_ERROR;
  }

  TIM_SlaveConfigTypeDef slave_config = {
    .SlaveMode = TIM_SLAVEMODE_EXTERNAL1,
    .InputTrigger = TIM_TS_ITR1, // will be clocked by TIM2
    .TriggerPrescaler = TIM_TRIGGERPRESCALER_DIV1
  };
  if (HAL_TIM_SlaveConfigSynchronization(&handle, &slave_config) != HAL_OK) {
    return HAL_ERROR;
  }

  __TIM1_HANDLE = handle;
  return HAL_OK;
}

HAL_StatusTypeDef TIM1_teardown(void) {
  if (HAL_TIM_Base_DeInit(&__TIM1_HANDLE) != HAL_OK) {
    return HAL_ERROR;
  }

  return HAL_OK;
}

HAL_StatusTypeDef timer_start(void) {
  if (HAL_TIM_Base_Start(&__TIM1_HANDLE) != HAL_OK) return HAL_ERROR; // start slave timer first
  if (HAL_TIM_Base_Start(&__TIM2_HANDLE) != HAL_OK) return HAL_ERROR;
  return HAL_OK;
}

HAL_StatusTypeDef timer_stop(void) {
  if (HAL_TIM_Base_Stop(&__TIM2_HANDLE) != HAL_OK) return HAL_ERROR;
  if (HAL_TIM_Base_Stop(&__TIM1_HANDLE) != HAL_OK) return HAL_ERROR; // stop slave timer last
  return HAL_OK;
}

uint64_t timer_read(void) {
  uint32_t high, high_again;
  uint32_t low;

  do {
    high = __HAL_TIM_GetCounter(&__TIM1_HANDLE);
    low = __HAL_TIM_GetCounter(&__TIM2_HANDLE);
    high_again = __HAL_TIM_GetCounter(&__TIM1_HANDLE);
  } while (high != high_again);

  return ((uint64_t) high << 32) | low;
}

uint64_t timer_delta(uint64_t begin, uint64_t end) {
  /*
  timer values overflow at 48 bits, not 64, so we can't rely on default uint64_t overflow behavior
  to remedy this, if we detect that end < begin, then we can conclude that end must have overflowed, so
  we simply add (2**48) to the end timestamp before doing computation 
  */

  return (end >= begin) ?
    end - begin :
    (end + (1ull<<48)) - begin;
}