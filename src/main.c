#include "main.h"

static void _error_handler(void) {
  // unrecoverable error, loop here indefinitely
  __disable_irq();
  while (true);
}

int main(void) {
  // HAL and interrupt initialization
  HAL_Init();
  __enable_irq();

  // peripheral initialization
  if (sysclk_config() != HAL_OK) _error_handler();
  print_config();
  if (
    TIM2_init() != HAL_OK
    || TIM1_init() != HAL_OK
    || timer_start() != HAL_OK) _error_handler();
  if (I2C1_init() != HAL_OK) _error_handler();
  if (SPI1_init() != HAL_OK) _error_handler();
  if (UART2_init() != HAL_OK) _error_handler();
  printf("LOG - Peripheral initialization complete\n");

  // start the game
  game_start();
  return 0;
}