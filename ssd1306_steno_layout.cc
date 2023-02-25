//---------------------------------------------------------------------------

#include "ssd1306.h"

//---------------------------------------------------------------------------

struct StenoLayoutData {
  uint8_t type;
  uint8_t x;
};

//---------------------------------------------------------------------------

#if JAVELIN_OLED_DRIVER == 1306

//---------------------------------------------------------------------------

#if JAVELIN_OLED_WIDTH == 128 && JAVELIN_OLED_HEIGHT == 64

constexpr StenoLayoutData LAYOUT_DATA[] = {
    {0, 5},   // #
    {1, 5},   // S
    {2, 17},  // T
    {3, 17},  // K
    {2, 29},  // P
    {3, 29},  // W
    {2, 41},  // H
    {3, 41},  // R
    {4, 33},  // A
    {4, 45},  // O
    {1, 53},  // *
    {4, 61},  // E
    {4, 73},  // U
    {2, 65},  // F
    {3, 65},  // R
    {2, 77},  // P
    {3, 77},  // B
    {2, 89},  // L
    {3, 89},  // G
    {2, 101}, // T
    {3, 101}, // S
    {2, 113}, // D
    {3, 113}, // Z
};

constexpr uint32_t type1OffData[] = {
    0xfffffc00, 0x000001ff, // Row 0
    0x00000400, 0x00000200, // Row 1
    0x00000400, 0x00000400, // Row 2
    0x00000400, 0x00000800, // Row 3
    0x00000400, 0x00000800, // Row 4
    0x00000400, 0x00000800, // Row 5
    0x00000400, 0x00000800, // Row 6
    0x00000400, 0x00000400, // Row 7
    0x00000400, 0x00000200, // Row 8
    0xfffffc00, 0x000001ff, // Row 9
};

constexpr uint32_t type1OnData[] = {
    0xfffffc00, 0x000001ff, // Row 0
    0xfffffc00, 0x000003ff, // Row 1
    0xfffffc00, 0x000007ff, // Row 2
    0xfffffc00, 0x00000fff, // Row 3
    0xfffffc00, 0x00000fff, // Row 4
    0xfffffc00, 0x00000fff, // Row 5
    0xfffffc00, 0x00000fff, // Row 6
    0xfffffc00, 0x000007ff, // Row 7
    0xfffffc00, 0x000003ff, // Row 8
    0xfffffc00, 0x000001ff, // Row 9
};

constexpr uint32_t type3OffData[] = {
    0xf0000000, 0x000001ff, // Row 0
    0x10000000, 0x00000200, // Row 1
    0x10000000, 0x00000400, // Row 2
    0x10000000, 0x00000800, // Row 3
    0x10000000, 0x00000800, // Row 4
    0x10000000, 0x00000800, // Row 5
    0x10000000, 0x00000800, // Row 6
    0x10000000, 0x00000400, // Row 7
    0x10000000, 0x00000200, // Row 8
    0xf0000000, 0x000001ff, // Row 9
};

constexpr uint32_t type3OnData[] = {
    0xf0000000, 0x000001ff, // Row 0
    0xf0000000, 0x000003ff, // Row 1
    0xf0000000, 0x000007ff, // Row 2
    0xf0000000, 0x00000fff, // Row 3
    0xf0000000, 0x00000fff, // Row 4
    0xf0000000, 0x00000fff, // Row 5
    0xf0000000, 0x00000fff, // Row 6
    0xf0000000, 0x000007ff, // Row 7
    0xf0000000, 0x000003ff, // Row 8
    0xf0000000, 0x000001ff, // Row 9
};

constexpr uint32_t type4OffData[] = {
    0x0fff8000, 0x10008000, 0x20008000, 0x40008000, 0x40008000,
    0x40008000, 0x40008000, 0x20008000, 0x10008000, 0x0fff8000,
};

constexpr uint32_t type4OnData[] = {
    0x0fff8000, 0x1fff8000, 0x3fff8000, 0x7fff8000, 0x7fff8000,
    0x7fff8000, 0x7fff8000, 0x3fff8000, 0x1fff8000, 0x0fff8000,
};

void Ssd1306::Ssd1306Data::DrawStenoLayout(StenoStroke stroke) {
  if (!available) {
    return;
  }

  Clear();

  for (size_t i = 0; i < sizeof(LAYOUT_DATA) / sizeof(*LAYOUT_DATA); ++i) {
    int x = LAYOUT_DATA[i].x;
    if (stroke.GetKeyState() & (1 << i)) {
      // On
      switch (LAYOUT_DATA[i].type) {
      case 0: {
        uint8_t *p = &buffer8[x * (JAVELIN_OLED_HEIGHT / 8)];
        for (int i = 0; i < 118; ++i) {
          *p = 0xfe;
          p += (JAVELIN_OLED_HEIGHT / 8);
        }
        break;
      }
      case 1: {
        uint32_t *p = &buffer32[x * (JAVELIN_OLED_HEIGHT / 32)];
        for (uint32_t d : type1OnData) {
          *p++ |= d;
        }
        break;
      }
      case 2: {
        uint32_t *p = &buffer32[x * (JAVELIN_OLED_HEIGHT / 32)];
        for (int i = 0; i < 10; ++i) {
          *p |= 0x03fffc00;
          p += (JAVELIN_OLED_HEIGHT / 32);
        }
        break;
      }
      case 3: {
        uint32_t *p = &buffer32[x * (JAVELIN_OLED_HEIGHT / 32)];
        for (uint32_t d : type3OnData) {
          *p++ |= d;
        }
        break;
      }
      case 4: {
        uint32_t *p = &buffer32[x * (JAVELIN_OLED_HEIGHT / 32)] + 1;
        for (uint32_t d : type4OnData) {
          *p |= d;
          p += (JAVELIN_OLED_HEIGHT / 32);
        }
        break;
      }
      }
    } else {
      // Off
      switch (LAYOUT_DATA[i].type) {
      case 0: {
        uint8_t *p = &buffer8[x * (JAVELIN_OLED_HEIGHT / 8)];
        *p = 0xfe;
        p += (JAVELIN_OLED_HEIGHT / 8);
        for (int i = 0; i < 116; ++i) {
          *p = 0x82;
          p += (JAVELIN_OLED_HEIGHT / 8);
        }
        *p = 0xfe;
        break;
      }
      case 1: {
        uint32_t *p = &buffer32[x * (JAVELIN_OLED_HEIGHT / 32)];
        for (uint32_t d : type1OffData) {
          *p++ |= d;
        }
        break;
      }
      case 2: {
        uint32_t *p = &buffer32[x * (JAVELIN_OLED_HEIGHT / 32)];
        *p |= 0x03fffc00;
        p += (JAVELIN_OLED_HEIGHT / 32);

        for (int i = 0; i < 8; ++i) {
          *p |= 0x002000400;
          p += (JAVELIN_OLED_HEIGHT / 32);
        }
        *p |= 0x03fffc00;
        break;
      }
      case 3: {
        uint32_t *p = &buffer32[x * (JAVELIN_OLED_HEIGHT / 32)];
        for (uint32_t d : type3OffData) {
          *p++ |= d;
        }
        break;
      }
      case 4: {
        uint32_t *p = &buffer32[x * (JAVELIN_OLED_HEIGHT / 32)] + 1;
        for (uint32_t d : type4OffData) {
          *p |= d;
          p += (JAVELIN_OLED_HEIGHT / 32);
        }
        break;
      }
      }
    }
  }
}

#else

void Ssd1306::Ssd1306Data::DrawStenoLayout(StenoStroke stroke) {
  // Not supported.
}

#endif

//---------------------------------------------------------------------------

#endif // JAVELIN_OLED_DRIVER == 1306

//---------------------------------------------------------------------------
