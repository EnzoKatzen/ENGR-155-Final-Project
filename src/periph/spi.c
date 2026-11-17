#include "periph/spi.h"

SPI_HandleTypeDef __SPI1_HANDLE = {};
static uint8_t _SPI1_rx[3];
volatile uint32_t SPI1_data = 0;
volatile bool SPI1_data_pending = false;

HAL_StatusTypeDef SPI1_init(void) { 
  GPIO_InitTypeDef gpio_config = {
    // PA4 as SPI1_NSS
    // PA5 as SPI1_SCK
    // PA6 as SPI1_MISO
    // PA12 as SPI1_MOSI
    .Pin = GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_12,
    .Mode = GPIO_MODE_AF_PP,
    .Speed = GPIO_SPEED_FREQ_VERY_HIGH,
    .Pull = GPIO_NOPULL,
    .Alternate = GPIO_AF5_SPI1
  };
  HAL_GPIO_Init(GPIOA, &gpio_config);

  SPI_HandleTypeDef handle = {
    .Instance = SPI1,
    .Init = {
      .Mode = SPI_MODE_SLAVE,
      .Direction = SPI_DIRECTION_2LINES,
      .DataSize = SPI_DATASIZE_6BIT,
      .CLKPolarity = SPI_POLARITY_HIGH,
      .CLKPhase = SPI_PHASE_2EDGE,
      .NSS = SPI_NSS_HARD_INPUT,
      .NSSPMode = SPI_NSS_PULSE_DISABLE,
      .FirstBit = SPI_FIRSTBIT_MSB
    }
  };
  if (HAL_SPI_Init(&handle) != HAL_OK) {
    return HAL_ERROR;
  }

  HAL_NVIC_SetPriority(SPI1_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(SPI1_IRQn);

  __SPI1_HANDLE = handle;
  return HAL_OK;
}

HAL_StatusTypeDef SPI1_teardown(void) {
  HAL_NVIC_DisableIRQ(SPI1_IRQn);

  if (HAL_SPI_DeInit(&__SPI1_HANDLE) != HAL_OK) {
    return HAL_ERROR;
  }

  HAL_GPIO_DeInit(GPIOA, GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_12);
  return HAL_OK;
}

HAL_StatusTypeDef SPI1_arm(void) {
  // this function "arms" the SPI1 peripheral such that the next time the master sends data, an interrupt is fired
  // you must call this after every SPI transaction
  return HAL_SPI_Receive_IT(&__SPI1_HANDLE, _SPI1_rx, 3);
}

void SPI1_IRQHandler(void) {
  HAL_SPI_IRQHandler(&__SPI1_HANDLE);
}

void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef* handle) {
  // invoked by the HAL whenever SPI receives something
  if (handle->Instance == SPI1) {
    SPI1_data =
      ((uint32_t) (_SPI1_rx[0] & 0b111111) << 12)
      | ((uint32_t) (_SPI1_rx[1] & 0b111111) << 6)
      | ((uint32_t) (_SPI1_rx[2] & 0b111111));
    SPI1_data_pending = true;
  }
}