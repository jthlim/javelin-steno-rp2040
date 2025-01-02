//---------------------------------------------------------------------------

#include "rp2040_encoder_state.h"
#include "javelin/button_script_manager.h"
#include "javelin/clock.h"
#include "javelin/console.h"
#include <hardware/gpio.h>
#include <hardware/timer.h>

//---------------------------------------------------------------------------

#if JAVELIN_ENCODER

Rp2040EncoderState Rp2040EncoderState::instance;

//---------------------------------------------------------------------------

static void InitializePin(int pin) {
  gpio_init(pin);
  gpio_set_dir(pin, false);
  gpio_pull_up(pin);
}

void Rp2040EncoderState::Initialize() {
  for (size_t i = 0; i < LOCAL_ENCODER_COUNT; ++i) {
    InitializePin(ENCODER_PINS[i].a);
    InitializePin(ENCODER_PINS[i].b);
    busy_wait_us_32(10);
    instance.lastEncoderStates[i] = gpio_get(ENCODER_PINS[i].a);
  }
}

void Rp2040EncoderState::CallScript(size_t encoderIndex, int delta) {
  constexpr size_t ENCODER_SCRIPT_OFFSET = (2 + BUTTON_COUNT * 2);
  const size_t scriptIndex =
      ENCODER_SCRIPT_OFFSET + 2 * encoderIndex + ((delta < 0) ? 1 : 0);
  ButtonScriptManager::GetInstance().ExecuteScriptIndex(
      scriptIndex, Clock::GetMilliseconds(), &delta, 1);
}

void Rp2040EncoderState::UpdateInternal() {
  for (size_t i = 0; i < LOCAL_ENCODER_COUNT; ++i) {
    const int newValue = gpio_get(ENCODER_PINS[i].a);
    if (newValue == lastEncoderStates[i]) {
      continue;
    }
    lastEncoderStates[i] = newValue;

#if JAVELIN_ENCODER_TRIGGER_ON_HIGH_ONLY
    if (newValue == 0) {
      continue;
    }
#endif
    const int delta = gpio_get(ENCODER_PINS[i].b) == newValue ? 1 : -1;
    deltas[i + JAVELIN_ENCODER_LOCAL_OFFSET] += delta;
#if JAVELIN_SPLIT && !JAVELIN_SPLIT_IS_MASTER
    CallScript(i + JAVELIN_ENCODER_LOCAL_OFFSET, delta);
#endif
  }

#if !JAVELIN_SPLIT || JAVELIN_SPLIT_IS_MASTER
  for (size_t i = 0; i < JAVELIN_ENCODER_COUNT; ++i) {
    const int delta = deltas[i];
    if (delta == 0) {
      continue;
    }
    deltas[i] = 0;
    CallScript(i, delta);
  }
#endif
}

bool Rp2040EncoderState::HasAnyDelta() const {
  for (const int8_t delta : deltas) {
    if (delta != 0) {
      return true;
    }
  }
  return false;
}

void Rp2040EncoderState::OnDataReceived(const void *data, size_t length) {
  const int8_t *receivedDeltas = (const int8_t *)data;
  for (size_t i = 0; i < JAVELIN_ENCODER_COUNT; ++i) {
    deltas[i] += receivedDeltas[i];
  }
}

void Rp2040EncoderState::UpdateBuffer(TxBuffer &buffer) {
  if (!HasAnyDelta()) {
    return;
  }

  if (buffer.Add(SplitHandlerId::ENCODER, deltas, sizeof(deltas))) {
    memset(deltas, 0, sizeof(deltas));
  }
}

//---------------------------------------------------------------------------

#endif // JAVELIN_ENCODER

//---------------------------------------------------------------------------