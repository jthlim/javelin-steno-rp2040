//---------------------------------------------------------------------------

#pragma once
#include JAVELIN_BOARD_CONFIG
#include "javelin/debounce.h"
#include "javelin/split/split.h"

//---------------------------------------------------------------------------

#if JAVELIN_ENCODER

class Rp2040EncoderState
#if JAVELIN_SPLIT
#if JAVELIN_SPLIT_IS_MASTER
    : public SplitRxHandler
#else
    : public SplitTxHandler
#endif
#endif
{
public:
  static void Initialize();

  static void Update() { instance.UpdateInternal(); }

  static void RegisterTxHandler() {
#if JAVELIN_SPLIT && !JAVELIN_SPLIT_IS_MASTER
    Split::RegisterTxHandler(&instance);
#endif
  }
  static void RegisterRxHandler() {
#if JAVELIN_SPLIT && JAVELIN_SPLIT_IS_MASTER
    Split::RegisterRxHandler(SplitHandlerId::ENCODER, &instance);
#endif
  }

private:
  static constexpr size_t LOCAL_ENCODER_COUNT =
      sizeof(ENCODER_PINS) / sizeof(*ENCODER_PINS);

  int8_t deltas[JAVELIN_ENCODER_COUNT];
  GlobalDeferredDebounce<uint8_t, 1> lastEncoderStates[LOCAL_ENCODER_COUNT];

  static Rp2040EncoderState instance;

  void UpdateInternal();

  bool HasAnyDelta() const;

  void OnDataReceived(const void *data, size_t length);
  void UpdateBuffer(TxBuffer &buffer);

  void CallScript(size_t encoderIndex, int delta);
};

#else

class Rp2040EncoderState {
public:
  static void Initialize() {}
  static void Update() {}
  static void RegisterTxHandler() {}
  static void RegisterRxHandler() {}
};

#endif

//---------------------------------------------------------------------------
