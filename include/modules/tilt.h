#pragma once

#include "stdlib.h"
#include "stdbool.h"
#include "stdio.h"
#include "math.h"

#include "stm32l4xx_hal.h"
#include "periph/tim.h"

#include "components/accel.h"

typedef struct {
  uint64_t last_tick;

  float tilt_danger;

  struct {
    uint32_t cooldown;
    bool pending;
  } strike;
} TiltModule_HandleTypeDef;

void tilt_module_update(TiltModule_HandleTypeDef* module, Accelerometer_HandleTypeDef* const imu);