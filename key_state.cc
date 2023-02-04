//---------------------------------------------------------------------------

#include "key_state.h"
#include "javelin/console.h"
#include <hardware/gpio.h>
#include <hardware/structs/ioqspi.h>
#include <hardware/structs/sio.h>
#include <hardware/sync.h>
#include <hardware/timer.h>
#include <pico/stdlib.h>

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

#if defined(BOOTSEL_BUTTON_INDEX)
// Picoboard has a button attached to the flash CS pin, which the bootrom
// checks, and jumps straight to the USB bootcode if the button is pressed
// (pulling flash CS low). We can check this pin in by jumping to some code in
// SRAM (so that the XIP interface is not required), floating the flash CS
// pin, and observing whether it is pulled low.
//
// This doesn't work if others are trying to access flash at the same time,
// e.g. XIP streamer, or the other core.
bool __no_inline_not_in_flash_func(get_bootsel_button)() {
  const int CS_PIN_INDEX = 1;

  // Must disable interrupts, as interrupt handlers may be in flash, and flash
  // access it about to be disabled temporarily.
  uint32_t flags = save_and_disable_interrupts();

  // Set chip select to Hi-Z
  hw_write_masked(&ioqspi_hw->io[CS_PIN_INDEX].ctrl,
                  GPIO_OVERRIDE_LOW << IO_QSPI_GPIO_QSPI_SS_CTRL_OEOVER_LSB,
                  IO_QSPI_GPIO_QSPI_SS_CTRL_OEOVER_BITS);

  // Note that no sleep function in flash can be called right now.
  // This is a ~10us wait.
  for (volatile int i = 0; i < 125; ++i) {
  }

  // The HI GPIO registers in SIO can observe and control the 6 QSPI pins.
  // Note the button pulls the pin *low* when pressed.
  bool buttonState = (sio_hw->gpio_hi_in & (1u << CS_PIN_INDEX)) == 0;

  // Restore the state of chip select
  hw_write_masked(&ioqspi_hw->io[CS_PIN_INDEX].ctrl,
                  GPIO_OVERRIDE_NORMAL << IO_QSPI_GPIO_QSPI_SS_CTRL_OEOVER_LSB,
                  IO_QSPI_GPIO_QSPI_SS_CTRL_OEOVER_BITS);

  restore_interrupts(flags);

  return buttonState;
}
#endif

ButtonState KeyState::Read() {
  ButtonState state;
  state.ClearAll();

  for (int r = 0; r < sizeof(ROW_PINS); ++r) {
    gpio_put_masked(ROW_PIN_MASK, ROW_PIN_MASK & ~(1 << ROW_PINS[r]));
    // Seems to work solidly with 2us wait. Use 10 for safety.
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

#if defined(BOOTSEL_BUTTON_INDEX)
  if (get_bootsel_button()) {
    state.Set(BOOTSEL_BUTTON_INDEX);
  }
#endif

  return state;
}

//---------------------------------------------------------------------------
