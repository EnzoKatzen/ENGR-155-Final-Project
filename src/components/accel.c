#include "components/accel.h"

static const uint8_t ACCEL_REG_X_OUT = 0x00;
static const uint8_t ACCEL_REG_Y_OUT = 0x01;
static const uint8_t ACCEL_REG_Z_OUT = 0x02;
static const uint8_t ACCEL_REG_TILT  = 0x03;
static const uint8_t ACCEL_REG_MODE  = 0x07;
static const uint8_t ACCEL_REG_SR    = 0x08;
static const uint8_t ACCEL_REG_PDET  = 0x09;
static const uint8_t ACCEL_REG_PD    = 0x0A;

static float _accel_conversion(uint8_t data) {
  // convert data from the imu registers into a float that represents acceleration in g
  // value is a 6-bit signed twos-complement integer
  int8_t value = (data & 0b11111) + ((data & (1<<5)) ? -32 : 0);
  return (1.5f / 32.0f) * value;
}

HAL_StatusTypeDef accel_write_mode(Accelerometer_HandleTypeDef* handle) {
  // update the active/standby mode of the imu
  if (handle == NULL || handle->bus == NULL) {
    return HAL_ERROR;
  }

  uint8_t payload[] = {
    handle->config.active_mode ? 0b1 : 0b0 // 0x07 - MODE
  };

  return HAL_I2C_Mem_Write(
    handle->bus,
    handle->address << 1,
    ACCEL_REG_MODE,
    I2C_MEMADD_SIZE_8BIT,
    payload,
    sizeof(payload),
    100
  );
}

HAL_StatusTypeDef accel_read_data(Accelerometer_HandleTypeDef* handle) {
  // reads sensor data and places the values into the data member of the struct
  if (handle == NULL || handle->bus == NULL) {
    return HAL_ERROR;
  }

  uint8_t data[4];

  HAL_StatusTypeDef result = HAL_I2C_Mem_Read(
    handle->bus,
    handle->address << 1,
    ACCEL_REG_X_OUT,
    I2C_MEMADD_SIZE_8BIT,
    data,
    sizeof(data),
    100
  );
  if (result != HAL_OK) {
    return result;
  }

  if (
    // bit 6 means the sensor was trying to update the register while we are reading it, so we have to read it again
    data[0] & (1<<6)
    || data[1] & (1<<6)
    || data[2] & (1<<6)
    || data[3] & (1<<6)
  ) {
    return HAL_BUSY;
  }

  handle->data.accel.x = _accel_conversion(data[0]);
  handle->data.accel.y = _accel_conversion(data[1]);
  handle->data.accel.z = _accel_conversion(data[2]);
  handle->data.shaken = (data[3] & (1<<7));
  handle->data.tilt.xy = (data[3] & (0b111 << 2)) >> 2;
  handle->data.tilt.z = (data[3] & 0b11);

  return HAL_OK;
}

HAL_StatusTypeDef accel_write_config(Accelerometer_HandleTypeDef* handle) {
  // writes configuration data to the imu (things like sampling rate and debounce filters)
  if (handle == NULL || handle->bus == NULL) {
    return HAL_ERROR;
  }

  uint8_t payload[] = {
    ((handle->config.tilt_debounce & 0b111) << 5) // FILT[2:0]
      | (handle->config.sample_rate & 0b111)      // AMSR[2:0]
  };

  return HAL_I2C_Mem_Write(
    handle->bus,
    handle->address << 1,
    ACCEL_REG_SR,
    I2C_MEMADD_SIZE_8BIT,
    payload,
    sizeof(payload),
    100
  );
}