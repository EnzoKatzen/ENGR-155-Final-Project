#include "periph/uart.h"

UART_HandleTypeDef __UART2_HANDLE = {};

HAL_StatusTypeDef UART2_init(void) {  
  GPIO_InitTypeDef gpio_config = {
    // PA2 as USART2_TX
    // PA15 as USART2_RX
    .Pin = GPIO_PIN_2,
    .Mode = GPIO_MODE_AF_PP,
    .Pull = GPIO_PULLUP,
    .Speed = GPIO_SPEED_FREQ_VERY_HIGH,
    .Alternate = GPIO_AF7_USART2
  };
  HAL_GPIO_Init(GPIOA, &gpio_config);
  gpio_config.Pin = GPIO_PIN_15;
  gpio_config.Alternate = GPIO_AF3_USART2;
  HAL_GPIO_Init(GPIOA, &gpio_config);

  UART_HandleTypeDef handle = {
    .Instance = USART2,
    .Init = {
      .BaudRate = 115200,
      .WordLength = UART_WORDLENGTH_8B,
      .StopBits = UART_STOPBITS_1,
      .Parity = UART_PARITY_NONE,
      .Mode = UART_MODE_TX_RX,
      .HwFlowCtl = UART_HWCONTROL_NONE,
      .OverSampling = UART_OVERSAMPLING_16,
      .OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE
    }
  };
  if (HAL_UART_Init(&handle) != HAL_OK) {
    return HAL_ERROR;
  }

  __UART2_HANDLE = handle;
  return HAL_OK;
}

HAL_StatusTypeDef UART2_teardown(void) {
  if (HAL_UART_DeInit(&__UART2_HANDLE) != HAL_OK) {
    return HAL_ERROR;
  }

  HAL_GPIO_DeInit(GPIOA, GPIO_PIN_2 | GPIO_PIN_15);
  return HAL_OK;
}