#include "game.h"

// configuration values
const uint32_t GAME_TICK_DELAY = 25; // ms

const uint8_t GAME_IMU_I2C_ADDRESS = 0x4C;
const uint8_t GAME_IMU_SAMPLE_RATE = 0; // update at 120 Hz
const uint8_t GAME_IMU_TILT_DEBOUNCE = 7; // 8 samples in a row must match

// game state variables
typedef struct {
  uint8_t mistakes;
} Game_StateTypeDef;
static Game_StateTypeDef _game_state = {};

// components
static Accelerometer_HandleTypeDef _game_imu = {};

// submodules
static TiltModule_HandleTypeDef _game_tilt_module = {};

// game logic functions
static void _game_init(void) {
  // accelerometer initialization
  _game_imu.bus = &__I2C1_HANDLE;
  _game_imu.address = GAME_IMU_I2C_ADDRESS;
  _game_imu.config.sample_rate = GAME_IMU_SAMPLE_RATE; // 120 Hz
  _game_imu.config.tilt_debounce = GAME_IMU_TILT_DEBOUNCE; // 8 samples in a row must match

  _game_imu.config.active_mode = false;
  accel_write_mode(&_game_imu);
  accel_write_config(&_game_imu);
  _game_imu.config.active_mode = true;
  accel_write_mode(&_game_imu);

  // tilt module initialization
  _game_tilt_module.last_tick = timer_read();
  _game_tilt_module.tilt_danger = 0.0f;
  _game_tilt_module.strike.cooldown = 0;
  _game_tilt_module.strike.pending = false;

  // game state initialization
  _game_state.mistakes = 0;
}

static void _sensor_update(void) {
  accel_read_data(&_game_imu);
}

static void _game_tick(void) {
  tilt_module_update(&_game_tilt_module, &_game_imu);
  printf("GAME - tilt danger: %.2f\n", _game_tilt_module.tilt_danger);

  if (_game_tilt_module.strike.pending) {
    printf("GAME - strike recorded from tilt module\n");
    _game_tilt_module.strike.pending = false;
  }
}

void game_start(void) {
  _game_init();
  printf("LOG - Game initialization complete\n");
  printf("LOG - Starting game...\n");

  while (true) {
    _sensor_update();
    _game_tick();
    HAL_Delay(GAME_TICK_DELAY);
  }
}