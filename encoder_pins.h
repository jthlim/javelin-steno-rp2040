//---------------------------------------------------------------------------

#pragma once
#include <stdint.h>

//---------------------------------------------------------------------------

struct EncoderPins {
  uint8_t a;
  uint8_t b;

#ifdef __cplusplus
  void Initialize() const;
  int ReadState() const;

private:
  static void InitializePin(int pin);
#endif
};

//---------------------------------------------------------------------------
