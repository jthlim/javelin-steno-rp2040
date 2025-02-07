//---------------------------------------------------------------------------

#pragma once
#include "javelin/button_state.h"
#include "javelin/cyclic_queue.h"
#include "javelin/debounce.h"

//---------------------------------------------------------------------------

class Rp2040ButtonState {
public:
  static void Initialize();
  static void Update() { instance.UpdateInternal(); }

  static bool HasData() { return instance.queue.IsNotEmpty(); }
  static bool IsEmpty() { return instance.queue.IsEmpty(); }
  static bool IsNotEmpty() { return instance.queue.IsNotEmpty(); }
  static constexpr TimedButtonState &Front() { return instance.queue.Front(); }
  static void RemoveFront() { instance.queue.RemoveFront(); }
  static size_t GetCount() { return instance.queue.GetCount(); }

private:
  static constexpr size_t QUEUE_COUNT = 64;

  uint32_t keyPressedTime;
  uint32_t rowMasks[2];
  CyclicQueue<TimedButtonState, QUEUE_COUNT> queue;
  GlobalDeferredDebounce<ButtonState> debouncer;

  static Rp2040ButtonState instance;

  void UpdateInternal();
  static ButtonState ReadInternal();
  static void ReadTouchCounters(uint32_t *counters);
};

//---------------------------------------------------------------------------
