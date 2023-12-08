//---------------------------------------------------------------------------

#include "rp2040_button_state.h"
#include "javelin/split/split.h"
#include <hardware/gpio.h>
#include <hardware/structs/ioqspi.h>
#include <hardware/structs/sio.h>
#include <hardware/sync.h>
#include <hardware/timer.h>

#include JAVELIN_BOARD_CONFIG

//---------------------------------------------------------------------------

#if JAVELIN_BUTTON_MATRIX

#if JAVELIN_SPLIT
#if defined(JAVELIN_SPLIT_IS_LEFT)
#if JAVELIN_SPLIT_IS_LEFT

#define COLUMN_PINS LEFT_COLUMN_PINS
#define COLUMN_PIN_MASK LEFT_COLUMN_PIN_MASK
#define ROW_PINS LEFT_ROW_PINS
#define ROW_PIN_MASK LEFT_ROW_PIN_MASK
#define KEY_MAP LEFT_KEY_MAP
const size_t COLUMN_PIN_COUNT = sizeof(LEFT_COLUMN_PINS);
const size_t ROW_PIN_COUNT = sizeof(LEFT_ROW_PINS);

#else // JAVELIN_SPLIT_IS_LEFT

#define COLUMN_PINS RIGHT_COLUMN_PINS
#define COLUMN_PIN_MASK RIGHT_COLUMN_PIN_MASK
#define ROW_PINS RIGHT_ROW_PINS
#define ROW_PIN_MASK RIGHT_ROW_PIN_MASK
#define KEY_MAP RIGHT_KEY_MAP
const size_t COLUMN_PIN_COUNT = sizeof(RIGHT_COLUMN_PINS);
const size_t ROW_PIN_COUNT = sizeof(RIGHT_ROW_PINS);

#endif // JAVELIN_SPLIT_IS_LEFT
#else  // defined(JAVELIN_SPLIT_IS_LEFT)

auto COLUMN_PINS = LEFT_COLUMN_PINS;
auto COLUMN_PIN_MASK = LEFT_COLUMN_PIN_MASK;
auto ROW_PINS = LEFT_ROW_PINS;
auto ROW_PIN_MASK = LEFT_ROW_PIN_MASK;
auto KEY_MAP = LEFT_KEY_MAP;
const size_t COLUMN_PIN_COUNT = sizeof(LEFT_COLUMN_PINS);
const size_t ROW_PIN_COUNT = sizeof(LEFT_ROW_PINS);

#endif // defined(JAVELIN_SPLIT_IS_LEFT)
#else  // JAVELIN_SPLIT

const size_t COLUMN_PIN_COUNT = sizeof(COLUMN_PINS);
const size_t ROW_PIN_COUNT = sizeof(ROW_PINS);

#endif

#endif // JAVELIN_BUTTON_MATRIX

#if JAVELIN_BUTTON_TOUCH
struct TouchPadState {
  uint32_t minimumCounter;
  uint32_t repeatedCount;
  uint32_t lastCounter;
};
TouchPadState touchPadStates[sizeof(BUTTON_TOUCH_PINS)];

#if !defined(JAVELIN_TOUCH_CALIBRATION_COUNT)
#define JAVELIN_TOUCH_CALIBRATION_COUNT 8
#endif

#endif

//---------------------------------------------------------------------------

void Rp2040ButtonState::Initialize() {
#if JAVELIN_BUTTON_MATRIX
#if JAVELIN_SPLIT
#if !defined(JAVELIN_SPLIT_IS_LEFT)
  if (!Split::IsLeft()) {
    COLUMN_PINS = RIGHT_COLUMN_PINS;
    COLUMN_PIN_MASK = RIGHT_COLUMN_PIN_MASK;
    ROW_PINS = RIGHT_ROW_PINS;
    ROW_PIN_MASK = RIGHT_ROW_PIN_MASK;
    KEY_MAP = RIGHT_KEY_MAP;
  }
#endif
#endif

  gpio_init_mask(COLUMN_PIN_MASK | ROW_PIN_MASK);
  gpio_set_dir_masked(COLUMN_PIN_MASK | ROW_PIN_MASK, ROW_PIN_MASK);

  for (size_t i = 0; i < COLUMN_PIN_COUNT; ++i) {
    gpio_pull_up(COLUMN_PINS[i]);
  }
  gpio_put_masked(ROW_PIN_MASK, ROW_PIN_MASK);

#endif

#if JAVELIN_BUTTON_PINS
  gpio_init_mask(BUTTON_PIN_MASK);
  gpio_set_dir_masked(BUTTON_PIN_MASK, 0);

  for (uint8_t pinAndPolarity : BUTTON_PINS) {
    uint8_t pin = pinAndPolarity & 0x7f;
    if (pinAndPolarity >> 7) {
      gpio_pull_down(pin);
    } else {
      gpio_pull_up(pin);
    }
  }
#endif

#if JAVELIN_BUTTON_TOUCH
  gpio_init_mask(BUTTON_TOUCH_PIN_MASK);
  for (uint8_t pin : BUTTON_TOUCH_PINS) {
    gpio_disable_pulls(pin);
    gpio_set_drive_strength(pin, GPIO_DRIVE_STRENGTH_12MA);
  }
  memset(touchPadStates, 0xff, sizeof(touchPadStates));

  gpio_set_dir_masked(BUTTON_TOUCH_PIN_MASK, BUTTON_TOUCH_PIN_MASK);
  gpio_put_masked(BUTTON_TOUCH_PIN_MASK, BUTTON_TOUCH_PIN_MASK);
#endif
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
static bool __no_inline_not_in_flash_func(isBootSelButtonPressed)() {
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

#if JAVELIN_BUTTON_TOUCH
void Rp2040ButtonState::ReadTouchCounters(uint32_t *counters) {
  for (size_t i = 0; i < sizeof(BUTTON_TOUCH_PINS); ++i) {
    uint8_t pin = BUTTON_TOUCH_PINS[i];
    gpio_set_dir(pin, false);

    size_t counter = 0;
    for (; counter < 100000; ++counter) {
      if (!gpio_get(pin)) {
        break;
      }
    }
    counters[i] = counter;
  }

  gpio_set_dir_masked(BUTTON_TOUCH_PIN_MASK, BUTTON_TOUCH_PIN_MASK);
  gpio_put_masked(BUTTON_TOUCH_PIN_MASK, BUTTON_TOUCH_PIN_MASK);
}
#endif

ButtonState Rp2040ButtonState::Read() {
  ButtonState state;
  state.ClearAll();

#if JAVELIN_BUTTON_MATRIX
  for (int r = 0; r < ROW_PIN_COUNT; ++r) {
    gpio_put_masked(ROW_PIN_MASK, ROW_PIN_MASK & ~(1 << ROW_PINS[r]));
    // Seems to work solidly with 2us wait. Use 10 for safety.
    busy_wait_us_32(10);

    int columnMask = gpio_get_all();
#pragma GCC unroll 1
    for (int c = 0; c < COLUMN_PIN_COUNT; ++c) {
      if (((columnMask >> COLUMN_PINS[c]) & 1) == 0) {
        int buttonIndex = KEY_MAP[r][c];
        if (buttonIndex >= 0) {
          state.Set(buttonIndex);
        }
      }
    }
  }

  gpio_put_masked(ROW_PIN_MASK, ROW_PIN_MASK);
#endif

#if JAVELIN_BUTTON_PINS
  int buttonMask = gpio_get_all();
#pragma GCC unroll 1
  for (size_t b = 0; b < sizeof(BUTTON_PINS); ++b) {
    uint8_t pinAndPolarity = BUTTON_PINS[b];
    uint8_t pin = pinAndPolarity & 0x7f;
    if (((buttonMask >> pin) & 1) == pinAndPolarity >> 7) {
      state.Set(b);
    }
  }
#endif

#if JAVELIN_BUTTON_TOUCH
  uint32_t counters[sizeof(BUTTON_TOUCH_PINS)];
  ReadTouchCounters(counters);

  for (size_t i = 0; i < sizeof(BUTTON_TOUCH_PINS); ++i) {
    uint32_t counter = counters[i];
    TouchPadState &padState = touchPadStates[i];
    if (counter == padState.lastCounter) {
      padState.repeatedCount++;
      if (padState.repeatedCount == JAVELIN_TOUCH_CALIBRATION_COUNT &&
          counter < padState.minimumCounter) {
        padState.minimumCounter = counter;
      }
    } else {
      padState.repeatedCount = 0;
    }
    padState.lastCounter = counter;

    bool isTouched =
        256 * counter >
        uint32_t(256 * BUTTON_TOUCH_THRESHOLD + 0.99) * padState.minimumCounter;
    if (isTouched) {
      state.Set(i);
    }
  }
#endif

#if defined(BOOTSEL_BUTTON_INDEX)
  if (isBootSelButtonPressed()) {
    state.Set(BOOTSEL_BUTTON_INDEX);
  }
#endif

  return state;
}

//---------------------------------------------------------------------------
