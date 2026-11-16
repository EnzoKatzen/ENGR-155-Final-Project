#include "periph/irq.h"

void SysTick_Handler(void) {
  HAL_IncTick();
}