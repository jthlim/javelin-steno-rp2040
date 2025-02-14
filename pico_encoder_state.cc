//---------------------------------------------------------------------------

#include "pico_encoder_state.h"
#include "javelin/base64.h"
#include "javelin/button_script_manager.h"
#include "javelin/clock.h"
#include "javelin/console.h"
#include "javelin/mem.h"
#include "javelin/split/split_console.h"
#include "javelin/str.h"
#include "javelin/writer.h"
#include <hardware/gpio.h>
#include <hardware/timer.h>

//---------------------------------------------------------------------------

#if JAVELIN_ENCODER

PicoEncoderState PicoEncoderState::instance;

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

PicoEncoderState::PicoEncoderState()
    : localEncoderCount(sizeof(ENCODER_PINS) / sizeof(*ENCODER_PINS)) {}

void PicoEncoderState::InitializeInternal() {

  // The encoder look up tables are used to provide a value when old state
  // changes to new state. The index in the lookup table is 4*newState +
  // oldState.
  //
  // The upper 4 bits represents the delta when shifting, but this is only
  // applied if the lower 4 bits, when ANDed with the previous state lookup,
  // has a non-zero value. In a typical encoder, the states, when rotated
  // clockwise, will go: 0 -> 1 -> 3 -> 2 -> 0
  //
  // The set-up of the AND bits effectively requires two consecutive motions
  // in the right direction. This eliminates unwanted movements or triggering.

  // clang-format off
#if JAVELIN_ENCODER_SPEED == 2
  static constexpr uint8_t INITIAL_DELTA_LUT[] = {
      0, 0xf4, 0x11, 0, //
      0x02, 0, 0, 0x04, //
      0x08, 0, 0, 0x01, //
      0, 0x12, 0xf8, 0, //
  };
#elif JAVELIN_ENCODER_SPEED == 1
  static constexpr int8_t INITIAL_DELTA_LUT[] = {
    0, 0xf2, 0x11, 0, //
    0, 0, 0, 0x02, //
    0, 0, 0, 0x01, //
    0, 0, 0, 0, //
};
#else
#error Encoder speed not defined
#endif
  // clang-format on

  for (size_t i = 0; i < localEncoderCount; ++i) {
    ENCODER_PINS[i].Initialize();
  }

  busy_wait_us_32(20);

  for (size_t i = 0; i < localEncoderCount; ++i) {
    Mem::Copy(encoderLUT[i + JAVELIN_ENCODER_LOCAL_OFFSET],
              (const int8_t *)INITIAL_DELTA_LUT, 16);
    const int state = ENCODER_PINS[i].ReadState();
    lastEncoderStates[i] = ENCODER_PINS[i].ReadState();
  }
}

void PicoEncoderState::CallScript(size_t encoderIndex, int delta) {
  if (delta == 0) {
    return;
  }

  constexpr size_t ENCODER_SCRIPT_OFFSET = (2 + BUTTON_COUNT * 2);
  const size_t scriptIndex =
      ENCODER_SCRIPT_OFFSET + 2 * encoderIndex + ((delta < 0) ? 1 : 0);
  ButtonScriptManager::GetInstance().ExecuteScriptIndex(
      scriptIndex, Clock::GetMilliseconds(), &delta, 1);
}

void PicoEncoderState::UpdateNoScriptCallInternal() {
  for (size_t i = 0; i < localEncoderCount; ++i) {
    const uint8_t newValue = ENCODER_PINS[i].ReadState();
    const uint8_t lastValue = lastEncoderStates[i].GetDebouncedState();
    const Debounced<uint8_t> debounced = lastEncoderStates[i].Update(newValue);
    if (!debounced.isUpdated) {
      continue;
    }

    // Console::Printf("%d -> %d\n\n", lastValue, newValue);

    const int newLutValue = encoderLUT[JAVELIN_ENCODER_LOCAL_OFFSET + i]
                                      [lastValue | (newValue << 2)];
    const int lastLutValue = lastLUT[i];
    int delta = newLutValue >> 4;
    if ((newLutValue & lastLutValue & 0xf) != 0) {
      deltas[JAVELIN_ENCODER_LOCAL_OFFSET + i] += delta;
    }
    lastLUT[i] = newLutValue;
  }
}

void PicoEncoderState::UpdateInternal() {
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

bool PicoEncoderState::HasAnyDelta() const {
  for (const int8_t delta : deltas) {
    if (delta != 0) {
      return true;
    }
  }
  return false;
}

void PicoEncoderState::OnDataReceived(const void *data, size_t length) {
  const int8_t *receivedDeltas = (const int8_t *)data;
  for (size_t i = 0; i < JAVELIN_ENCODER_COUNT; ++i) {
    deltas[i] += receivedDeltas[i];
  }
}

void PicoEncoderState::OnReceiveConnected() {
  for (size_t i = localEncoderCount; i < JAVELIN_ENCODER_COUNT; ++i) {
    SendConfigurationInfoToPair(i);
  }
}

void PicoEncoderState::UpdateBuffer(TxBuffer &buffer) {
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

// A string describing the LUT.
// First char is + or -, then a hex digit for the offset.
bool PicoEncoderState::SetConfiguration(size_t encoderIndex,
                                        const int8_t *data) {
  if (encoderIndex >= JAVELIN_ENCODER_COUNT) {
    return false;
  }

  Mem::Copy(encoderLUT[encoderIndex], data, 16);
  Mem::Clear(lastLUT);

#if JAVELIN_SPLIT && JAVELIN_SPLIT_IS_MASTER
  if (encoderIndex >= localEncoderCount && Connection::IsPairConnected()) {
    SendConfigurationInfoToPair(encoderIndex);
  }
#endif

  return true;
}

void PicoEncoderState::SendConfigurationInfoToPair(size_t encoderIndex) {
  BufferWriter consoleBuffer;
  consoleBuffer.Printf("set_encoder_configuration %zu %D\n", encoderIndex,
                       encoderLUT[encoderIndex],
                       sizeof(encoderLUT[encoderIndex]));
  const size_t length = consoleBuffer.GetCount() + 1;
  char *data = consoleBuffer.TerminateStringAndAdoptBuffer();
  SplitConsole::Add(data, length);
  free(data);
}

void PicoEncoderState::SetEncoderConfiguration_Binding(
    void *context, const char *commandLine) {

  const char *instructions =
      "Common configuration data:\n"
      "- Normal: \"APQRAAIAAAQIAAABABL4AA==\"\n"
      "- Reversed: \"ABTxAAIAAAQIAAABAPIYAA==\"\n"
      "- Phase Shift: \"AAgCABIAAPT4AAARAAEEAA==\"\n"
      "- Phase Shift Reversed: \"AAgCAPIAABQYAADxAAEEAA==\"\n"
      "- Half Speed: \"APIRAAAAAAIAAAABAAAAAA==\"\n"
      "- Half Speed Reversed: \"ABLxAAAAAAIAAAABAAAAAA==\"\n";

  const char *p = strchr(commandLine, ' ');
  if (!p) {
    Console::Printf("ERR missing encoder index and configuration data\n%s\n",
                    instructions);
    return;
  }

  int index;
  p = Str::ParseInteger(&index, p + 1, false);
  if (!p) {
    Console::Printf("ERR unable to parse encoder index\n\n");
    return;
  }

  if (*p != ' ') {
    Console::Printf("ERR missing configuration data\n%s\n", instructions);
    return;
  }

  uint8_t decodeBuffer[256];
  const size_t byteCount = Base64::Decode(decodeBuffer, (const uint8_t *)p + 1);
  if (byteCount != 16) {
    Console::Printf("ERR bad configuration data\n%s\n", instructions);
    return;
  }

  if (!instance.SetConfiguration(index, (int8_t *)decodeBuffer)) {
    Console::Printf("ERR unable to set configuration\n\n");
    return;
  }

  Console::SendOk();
}

//---------------------------------------------------------------------------

#endif // JAVELIN_ENCODER

//---------------------------------------------------------------------------