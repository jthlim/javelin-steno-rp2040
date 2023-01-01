//---------------------------------------------------------------------------

#include "key_state.h"
#include <hardware/gpio.h>
#include <hardware/timer.h>

#include JAVELIN_BOARD_CONFIG

//---------------------------------------------------------------------------

void KeyState::Init() {
  gpio_init_mask(COLUMN_PIN_MASK | ROW_PIN_MASK);
  gpio_set_dir_masked(COLUMN_PIN_MASK | ROW_PIN_MASK, ROW_PIN_MASK);

  for (uint8_t input : COLUMN_PINS) {
    gpio_pull_up(input);
  }
  gpio_put_masked(ROW_PIN_MASK, ROW_PIN_MASK);
}

ButtonState KeyState::Read() {
  ButtonState state;
  state.ClearAll();

#pragma GCC unroll 1
  for (int r = 0; r < sizeof(ROW_PINS); ++r) {
    gpio_put_masked(ROW_PIN_MASK, ROW_PIN_MASK & ~(1 << ROW_PINS[r]));
    busy_wait_us_32(10);

    int columnMask = gpio_get_all();
#pragma GCC unroll 1
    for (int c = 0; c < sizeof(COLUMN_PINS); ++c) {
      if (((columnMask >> COLUMN_PINS[c]) & 1) == 0) {
        int buttonIndex = KEY_MAP[r][c];
        if (buttonIndex >= 0) {
          state.Set(buttonIndex);
        }
      }
    }
  }

  gpio_put_masked(ROW_PIN_MASK, ROW_PIN_MASK);
  return state;
}

//---------------------------------------------------------------------------
