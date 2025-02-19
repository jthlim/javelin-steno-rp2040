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

  static size_t GetLocalEncoderCount() { return localEncoderCount; }
  static void SetLocalEncoderCount(size_t value) { localEncoderCount = value; }

  static void Update() { instance.UpdateInternal(); }
  static void UpdateNoScriptCall() { instance.UpdateNoScriptCallInternal(); }

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
  static size_t localEncoderCount;

  int8_t deltas[JAVELIN_ENCODER_COUNT];
#if JAVELIN_SPLIT && !JAVELIN_SPLIT_IS_MASTER
  int8_t lastDeltas[JAVELIN_ENCODER_COUNT];
#endif
  GlobalDeferredDebounce<uint8_t, 1> lastEncoderStates[JAVELIN_ENCODER_COUNT];

  static Rp2040EncoderState instance;

  void UpdateInternal();
  void UpdateNoScriptCallInternal();

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
  static void UpdateNoScriptCall() {}
  static void RegisterTxHandler() {}
  static void RegisterRxHandler() {}
};

#endif

//---------------------------------------------------------------------------
