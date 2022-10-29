//---------------------------------------------------------------------------

#include "key_state.h"
#include "uniV4/config.h"
#include <hardware/gpio.h>
#include <hardware/timer.h>

//---------------------------------------------------------------------------

void KeyState::Init() {
  gpio_init_mask(COLUMN_PIN_MASK | ROW_PIN_MASK);
  gpio_set_dir_masked(COLUMN_PIN_MASK | ROW_PIN_MASK, ROW_PIN_MASK);

  for (uint8_t input : COLUMN_PINS) {
    gpio_pull_up(input);
  }
  gpio_put_masked(ROW_PIN_MASK, ROW_PIN_MASK);
}

StenoKeyState KeyState::Read() {
  uint64_t state = 0;

  for (int r = 0; r < sizeof(ROW_PINS); ++r) {
    gpio_put(ROW_PINS[r], false);
    busy_wait_us_32(10);

    for (int c = 0; c < sizeof(COLUMN_PINS); ++c) {
      if (gpio_get(COLUMN_PINS[c]) == 0) {
        StenoKey key = KEY_MAP[r][c];
        if (key != StenoKey::NONE) {
          state |= (1 << (int)key);
        }
      }
    }

    gpio_put(ROW_PINS[r], true);
  }

  return StenoKeyState(state);
}

//---------------------------------------------------------------------------
