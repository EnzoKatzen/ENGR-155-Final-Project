#include "modules/tilt.h"

const float TILT_DANGER_MAX = 100.0f;
const float TILT_DANGER_THRESHOLD_DEGREES = 50.0f;
const float TILT_DANGER_INCREASE_FACTOR = 0.005f;
const float TILT_DANGER_DECREASE_FACTOR = 0.05f;
const uint32_t TILT_STRIKE_COOLDOWN = 4000;

void tilt_module_update(TiltModule_HandleTypeDef* module, Accelerometer_HandleTypeDef* const imu) {
  // calculate time elapsed
  uint64_t now_tick = timer_read();
  uint64_t delta_ms = timer_delta(module->last_tick, now_tick) / 1000;
  if (delta_ms > 1000) delta_ms = 1000; // clamp at 1000ms

  // calculate tilt deviation from threshold
  float abs_roll = fabsf(atan2f(imu->data.accel.x, imu->data.accel.z) * (180.0f / M_PI));
  float abs_pitch = fabsf(atan2f(imu->data.accel.y, imu->data.accel.z) * (180.0f / M_PI));
  float roll_dev = fmaxf(abs_roll - TILT_DANGER_THRESHOLD_DEGREES, 0.0f);
  float pitch_dev = fmaxf(abs_pitch - TILT_DANGER_THRESHOLD_DEGREES, 0.0f);
  float total_dev = hypotf(roll_dev, pitch_dev);

  // update tilt danger
  float punishment = (TILT_DANGER_INCREASE_FACTOR * total_dev - TILT_DANGER_DECREASE_FACTOR) * (float) delta_ms;
  module->tilt_danger += punishment;

  if (module->strike.cooldown < delta_ms) {
    module->strike.cooldown = 0;
  }
  else {
    module->strike.cooldown -= (uint32_t) delta_ms;
  }

  if (module->tilt_danger < 0.0f) {
    module->tilt_danger = 0.0f;
  }
  else if (module->tilt_danger > TILT_DANGER_MAX) {
    module->tilt_danger = TILT_DANGER_MAX;

    if (module->strike.cooldown == 0) {
      module->strike.pending = true;
      module->strike.cooldown = TILT_STRIKE_COOLDOWN;
    }
  }

  // update time
  module->last_tick = now_tick;
}