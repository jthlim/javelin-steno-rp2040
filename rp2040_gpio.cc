//---------------------------------------------------------------------------

#include "javelin/hal/gpio.h"
#include <hardware/gpio.h>

//---------------------------------------------------------------------------

void Gpio::SetInputPin(int pin, Pull pull) {
  gpio_init(pin);
  gpio_set_dir(pin, false);
  switch (pull) {
  case Pull::NONE:
    break;
  case Pull::DOWN:
    gpio_pull_down(pin);
    break;
  case Pull::UP:
    gpio_pull_up(pin);
    break;
  }
}

bool Gpio::GetPin(int pin) { return gpio_get(pin) != 0; }

void Gpio::SetPin(int pin, bool value) {
  gpio_init(pin);
  gpio_set_dir(pin, true);
  gpio_put(pin, value);
}

//---------------------------------------------------------------------------
