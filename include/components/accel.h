#pragma once

#include "stdlib.h"
#include "stdbool.h"

#include "stm32l4xx_hal.h"

typedef enum {
  ACCEL_TILT_UNKNOWN = 0b000,
  ACCEL_TILT_LEFT = 0b001,
  ACCEL_TILT_RIGHT = 0b010,
  ACCEL_TILT_FRONT = 0b101,
  ACCEL_TILT_BACK = 0b110,
  ACCEL_TILT_UP = 0b001,
  ACCEL_TILT_DOWN = 0b010
} Accelerometer_TiltTypeDef;

typedef struct {
  I2C_HandleTypeDef* bus;
  uint8_t address;

  struct { // output values from the sensor
    struct {
      float x;
      float y;
      float z;
    } accel;

    bool shaken;

    struct {
      Accelerometer_TiltTypeDef xy;
      Accelerometer_TiltTypeDef z;
    } tilt;
  } data;
  
  struct { // configuration settings for the sensor
    bool active_mode;
    uint8_t sample_rate;
    uint8_t tilt_debounce;
  } config;
} Accelerometer_HandleTypeDef;

HAL_StatusTypeDef accel_write_mode(Accelerometer_HandleTypeDef* handle);
HAL_StatusTypeDef accel_read_data(Accelerometer_HandleTypeDef* handle);
HAL_StatusTypeDef accel_write_config(Accelerometer_HandleTypeDef* handle);
// ALL METHODS ABOVE MODIFY STRUCT HANDLE IN-PLACE