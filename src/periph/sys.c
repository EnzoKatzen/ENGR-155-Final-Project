#include "periph/sys.h"

HAL_StatusTypeDef sysclk_config(void) {
  // enable all peripheral clocks
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_RCC_SYSCFG_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_TIM2_CLK_ENABLE();
  __HAL_RCC_TIM1_CLK_ENABLE();
  __HAL_RCC_I2C1_CLK_ENABLE();
  __HAL_RCC_SPI1_CLK_ENABLE();
  __HAL_RCC_USART2_CLK_ENABLE();
  __HAL_RCC_ADC_CLK_ENABLE();

  // run HSI at 80 MHz through the PLL
  RCC_OscInitTypeDef osc_config = {
    .OscillatorType = RCC_OSCILLATORTYPE_HSI,
    .HSIState = RCC_HSI_ON,
    .HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT,
    .PLL = {
      .PLLState = RCC_PLL_ON,
      .PLLSource = RCC_PLLSOURCE_HSI,
      .PLLM = 1,
      .PLLN = 10,
      .PLLP = 7,
      .PLLQ = 2,
      .PLLR = 2
    }
  };

  // use the PLL as the system clock
  RCC_ClkInitTypeDef clk_config = {
    .ClockType =
      RCC_CLOCKTYPE_SYSCLK
      | RCC_CLOCKTYPE_HCLK
      | RCC_CLOCKTYPE_PCLK1
      | RCC_CLOCKTYPE_PCLK2,
    .SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK,
    .AHBCLKDivider = RCC_SYSCLK_DIV1,
    .APB1CLKDivider = RCC_HCLK_DIV1,
    .APB2CLKDivider = RCC_HCLK_DIV1,
  };

  if (HAL_RCC_OscConfig(&osc_config) != HAL_OK) return HAL_ERROR;
  if (HAL_RCC_ClockConfig(&clk_config, FLASH_LATENCY_4) != HAL_OK) return HAL_ERROR;

  return HAL_OK;
}

void SysTick_Handler(void) {
  HAL_IncTick();
}

void print_config(void) {
  setvbuf(stdout, NULL, _IONBF, 0);
}

int _write(int file, char *ptr, int len) {
  (void) file;

  if (HAL_UART_Transmit(
    &__UART2_HANDLE,
    (uint8_t*) ptr,
    (uint16_t) len,
    HAL_MAX_DELAY
  ) != HAL_OK) {
    return -1;
  }

  return len;
}