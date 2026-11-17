#include "periph/adc.h"

ADC_HandleTypeDef __ADC1_HANDLE = {};

HAL_StatusTypeDef ADC1_init(void) {
  GPIO_InitTypeDef gpio_config = {
    // PA0 as ADC1 channel 5
    .Pin = GPIO_PIN_0,
    .Mode = GPIO_MODE_ANALOG_ADC_CONTROL,
    .Pull = GPIO_NOPULL 
  };
  HAL_GPIO_Init(GPIOA, &gpio_config);

  ADC_HandleTypeDef handle = {
    .Instance = ADC1,
    .Init = {
      .ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV2,
      .Resolution = ADC_RESOLUTION_12B,
      .DataAlign = ADC_DATAALIGN_RIGHT,
      .ScanConvMode = ADC_SCAN_DISABLE,
      .ContinuousConvMode = ENABLE,
      .DiscontinuousConvMode = DISABLE,
      .EOCSelection = ADC_EOC_SINGLE_CONV,
      .LowPowerAutoWait = DISABLE,
      .ExternalTrigConv = ADC_SOFTWARE_START,
      .ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE,
      .DMAContinuousRequests = DISABLE,
      .Overrun = ADC_OVR_DATA_OVERWRITTEN,
      .OversamplingMode = DISABLE
    }
  };
  if (
    HAL_ADC_Init(&handle) != HAL_OK
    || HAL_ADCEx_Calibration_Start(&handle, ADC_SINGLE_ENDED) != HAL_OK
  ) {
    return HAL_ERROR;
  }

  ADC_ChannelConfTypeDef channel_config = {
    .Channel = ADC_CHANNEL_5,
    .Rank = ADC_REGULAR_RANK_1,
    .SamplingTime = ADC_SAMPLETIME_47CYCLES_5,
    .SingleDiff = ADC_SINGLE_ENDED,
    .OffsetNumber = ADC_OFFSET_NONE,
    .Offset = 0
  };
  if (HAL_ADC_ConfigChannel(&handle, &channel_config) != HAL_OK) {
    return HAL_ERROR;
  }

  ADC_AnalogWDGConfTypeDef watchdog_config = {
    .WatchdogNumber = ADC_ANALOGWATCHDOG_1,
    .WatchdogMode = ADC_ANALOGWATCHDOG_SINGLE_REG,
    .Channel = ADC_CHANNEL_5,
    .ITMode = DISABLE,
    .LowThreshold = 1241, // (1V / 3.3V) * 4095 = 1241
    .HighThreshold = 2482 // (2V / 3.3V) * 4095 = 2482
  };
  if (HAL_ADC_AnalogWDGConfig(&handle, &watchdog_config) != HAL_OK) {
    return HAL_ERROR;
  }

  HAL_NVIC_SetPriority(ADC1_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(ADC1_IRQn);

  __ADC1_HANDLE = handle;
  return HAL_OK;
}

HAL_StatusTypeDef ADC1_teardown(void) {
  HAL_NVIC_DisableIRQ(ADC1_IRQn);

  if (HAL_ADC_DeInit(&__ADC1_HANDLE) != HAL_OK) {
    return HAL_ERROR;
  }

  return HAL_OK;
}

void ADC1_arm(void) {
  // make ADC1 fire one interrupt the next time it goes out of range, must be re-armed after each use
  __HAL_ADC_ENABLE_IT(&__ADC1_HANDLE, ADC_IT_AWD1);
}

void ADC1_IRQHandler(void) {
  HAL_ADC_IRQHandler(&__ADC1_HANDLE);
}

void HAL_ADC_LevelOutOfWindowCallback(ADC_HandleTypeDef* handle) {
  if (handle->Instance == ADC1) {
    __HAL_ADC_DISABLE_IT(handle, ADC_IT_AWD1);
    // this runs once whenever the ADC goes out of range
  }
}