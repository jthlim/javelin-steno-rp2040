//---------------------------------------------------------------------------

#include "rp2040_encoder_state.h"
#include "javelin/button_script_manager.h"
#include "javelin/clock.h"
#include <hardware/gpio.h>
#include <hardware/timer.h>

//---------------------------------------------------------------------------

#if JAVELIN_ENCODER

size_t Rp2040EncoderState::localEncoderCount =
    sizeof(ENCODER_PINS) / sizeof(*ENCODER_PINS);
Rp2040EncoderState Rp2040EncoderState::instance;

//---------------------------------------------------------------------------

void EncoderPins::Initialize() const {
  InitializePin(a);
  InitializePin(b);
}

void EncoderPins::InitializePin(int pin) {
  gpio_init(pin);
  gpio_set_dir(pin, GPIO_IN);
  gpio_pull_up(pin);
}

int EncoderPins::ReadState() const {
  return (gpio_get(a) ? 1 : 0) | (gpio_get(b) ? 2 : 0);
}

//---------------------------------------------------------------------------

void Rp2040EncoderState::Initialize() {
  for (size_t i = 0; i < localEncoderCount; ++i) {
    ENCODER_PINS[i].Initialize();
  }

  busy_wait_us_32(20);

  for (size_t i = 0; i < localEncoderCount; ++i) {
    instance.lastEncoderStates[i] = ENCODER_PINS[i].ReadState();
  }
}

void Rp2040EncoderState::CallScript(size_t encoderIndex, int delta) {
  if (delta == 0) {
    return;
  }

  constexpr size_t ENCODER_SCRIPT_OFFSET = (2 + BUTTON_COUNT * 2);
  const size_t scriptIndex =
      ENCODER_SCRIPT_OFFSET + 2 * encoderIndex + ((delta < 0) ? 1 : 0);
  ButtonScriptManager::GetInstance().ExecuteScriptIndex(
      scriptIndex, Clock::GetMilliseconds(), &delta, 1);
}

void Rp2040EncoderState::UpdateNoScriptCallInternal() {
  for (size_t i = 0; i < localEncoderCount; ++i) {
    const uint8_t newValue = ENCODER_PINS[i].ReadState();
    const uint8_t lastValue = lastEncoderStates[i].GetDebouncedState();
    const Debounced<uint8_t> debounced = lastEncoderStates[i].Update(newValue);
    if (!debounced.isUpdated) {
      continue;
    }

#if JAVELIN_ENCODER_SPEED == 2
    static constexpr int8_t DELTA_LUT[] = {
        0, -1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, -1, 0,
    };
#elif JAVELIN_ENCODER_SPEED == 1
    static constexpr int8_t DELTA_LUT[] = {
        0, -1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    };
#else
#error Encoder speed not defined
#endif
    const int delta = DELTA_LUT[lastValue | (newValue << 2)];
    deltas[JAVELIN_ENCODER_LOCAL_OFFSET + i] += delta;
  }
}

void Rp2040EncoderState::UpdateInternal() {
  UpdateNoScriptCallInternal();

#if JAVELIN_SPLIT && !JAVELIN_SPLIT_IS_MASTER

  for (size_t i = 0; i < localEncoderCount; ++i) {
    const int delta = deltas[JAVELIN_ENCODER_LOCAL_OFFSET + i];
    if (delta != lastDeltas[i]) {
      CallScript(JAVELIN_ENCODER_LOCAL_OFFSET + i, delta - lastDeltas[i]);
      lastDeltas[i] = delta;
    }
  }

#endif

#if !JAVELIN_SPLIT || JAVELIN_SPLIT_IS_MASTER
  for (size_t i = 0; i < JAVELIN_ENCODER_COUNT; ++i) {
    const int delta = deltas[i];
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
    Mem::Clear(deltas);
#if JAVELIN_SPLIT && !JAVELIN_SPLIT_IS_MASTER
    Mem::Clear(lastDeltas);
#endif
  }
}

//---------------------------------------------------------------------------

#endif // JAVELIN_ENCODER

//---------------------------------------------------------------------------