//---------------------------------------------------------------------------

#include "javelin/hal/gpio.h"
#include <hardware/gpio.h>
#include <hardware/pwm.h>

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

void Gpio::SetPinDutyCycle(int pin, int dutyCycle) {
  gpio_set_function(pin, GPIO_FUNC_PWM);
  uint32_t slice_num = pwm_gpio_to_slice_num(pin);
  pwm_set_wrap(slice_num, 99);
  pwm_set_enabled(slice_num, true);
  pwm_set_gpio_level(pin, dutyCycle);
}

//---------------------------------------------------------------------------
