#include "periph/spi.h"

SPI_HandleTypeDef __SPI1_HANDLE = {};

HAL_StatusTypeDef SPI1_init(void) { 
  GPIO_InitTypeDef gpio_config = {
    // PA5 as SPI1_SCK
    // PA6 as SPI1_MISO
    // PA7 as SPI1_MOSI
    .Pin = GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7,
    .Mode = GPIO_MODE_AF_PP,
    .Pull = GPIO_NOPULL,
    .Alternate = GPIO_AF5_SPI1
  };
  HAL_GPIO_Init(GPIOA, &gpio_config);

  SPI_HandleTypeDef handle = {
    .Instance = SPI1,
    .Init = {
      .Mode = SPI_MODE_MASTER,
      .Direction = SPI_DIRECTION_2LINES,
      .DataSize = SPI_DATASIZE_8BIT,
      .CLKPolarity = SPI_POLARITY_HIGH,
      .CLKPhase = SPI_PHASE_2EDGE,
      .NSS = SPI_NSS_SOFT,
      .BaudRatePrescaler = SPI_BAUDRATEPRESCALER_64,
      .FirstBit = SPI_FIRSTBIT_MSB
    }
  };
  if (HAL_SPI_Init(&handle) != HAL_OK) {
    return HAL_ERROR;
  }

  __SPI1_HANDLE = handle;
  return HAL_OK;
}

HAL_StatusTypeDef SPI1_teardown(void) {
  if (HAL_SPI_DeInit(&__SPI1_HANDLE) != HAL_OK) {
    return HAL_ERROR;
  }

  HAL_GPIO_DeInit(GPIOA, GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7);
  return HAL_OK;
}