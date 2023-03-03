//---------------------------------------------------------------------------

#pragma once
#include <stdint.h>
#include <stdlib.h>

//---------------------------------------------------------------------------

struct Font {
  uint16_t startUnicode; // Inclusive
  uint16_t endUnicode;   // Exclusive
  uint8_t width;         // 0 = proportional
  uint8_t height;        // Must be non-zero
  uint8_t baseline;      // y offset is adjusted by this value.
  uint8_t spacing;

  const uint16_t *glyphOffsets;

  // Glyph data has a width per character
  const uint8_t *glyphData;

  uint32_t GetCharacterWidth(uint32_t c) const;
  const uint8_t *GetCharacterData(uint32_t c) const;
  uint32_t GetStringWidth(const char *p) const;

  static const Font DEFAULT;
  static const Font LARGE_DIGITS;
};

//---------------------------------------------------------------------------