//---------------------------------------------------------------------------

#include "javelin/font/monochrome/font.h"
#include "st7789.h"

//---------------------------------------------------------------------------

const uint8_t PAPER_TAPE_FONT_DATA[] = {
    5, 0x14, 0x3e, 0x14, 0x3e, 0x14, // #
    4, 0x24, 0x4a, 0x52, 0x24,       // S
    5, 0x02, 0x02, 0x7e, 0x02, 0x02, // T
    4, 0x7e, 0x18, 0x24, 0x42,       // K
    4, 0x7e, 0x12, 0x12, 0x0c,       // P
    5, 0x3e, 0x40, 0x30, 0x40, 0x3e, // W
    4, 0x7e, 0x10, 0x10, 0x7e,       // H
    4, 0x7e, 0x12, 0x12, 0x6c,       // R
    4, 0x7c, 0x12, 0x12, 0x7c,       // A
    4, 0x3c, 0x42, 0x42, 0x3c,       // O
    5, 0x24, 0x18, 0x7e, 0x18, 0x24, // *
    4, 0x7e, 0x52, 0x52, 0x42,       // E
    4, 0x3e, 0x40, 0x40, 0x3e,       // U
    4, 0x7e, 0x12, 0x12, 0x02,       // F
    4, 0x7e, 0x12, 0x12, 0x6c,       // R
    4, 0x7e, 0x12, 0x12, 0x0c,       // P
    4, 0x7e, 0x52, 0x52, 0x2c,       // B
    4, 0x7e, 0x40, 0x40, 0x40,       // L
    4, 0x3c, 0x42, 0x52, 0x34,       // G
    5, 0x02, 0x02, 0x7e, 0x02, 0x02, // T
    4, 0x24, 0x4a, 0x52, 0x24,       // S
    4, 0x7e, 0x42, 0x42, 0x3c,       // D
    4, 0x62, 0x52, 0x4a, 0x46,       // Z
};

//---------------------------------------------------------------------------

#if JAVELIN_DISPLAY_DRIVER == 7789

//---------------------------------------------------------------------------

void St7789::St7789Data::DrawPaperTape(const StenoStroke *strokes,
                                       size_t length) {
  // if (!available) {
  //   return;
  // }

  Clear();

  // #if JAVELIN_DISPLAY_WIDTH >= 64
  //   constexpr size_t MAXIMUM_STROKES = JAVELIN_DISPLAY_HEIGHT / 8;
  //   size_t startingStroke =
  //       length > MAXIMUM_STROKES ? length - MAXIMUM_STROKES : 0;
  //   size_t lineOffset = length < MAXIMUM_STROKES ? MAXIMUM_STROKES - length :
  //   0;

  //   if (lineOffset >= 2) {
  //     DrawLine(0, 0, JAVELIN_DISPLAY_WIDTH, 0);
  //     DrawLine(JAVELIN_DISPLAY_WIDTH - 1, 0, JAVELIN_DISPLAY_WIDTH - 1, 15);
  //     DrawLine(0, 15, JAVELIN_DISPLAY_WIDTH, 15);
  //     DrawLine(0, 0, 0, 15);
  //     DrawText(JAVELIN_DISPLAY_WIDTH / 2, 12, &Font::DEFAULT,
  //              TextAlignment::MIDDLE, "Paper Tape");
  //   }

  //   for (size_t i = startingStroke; i < length; ++i) {
  //     uint32_t stroke = strokes[i].GetKeyState();
  //     uint8_t *p = &buffer8[lineOffset++];
  //     const uint8_t *f = PAPER_TAPE_FONT_DATA;

  //     for (size_t i = 0; i < 23; ++i) {
  //       int width = *f++;
  //       if (stroke & (1 << i)) {
  //         for (int x = 0; x < width; ++x) {
  //           *p = *f++;
  //           p += JAVELIN_DISPLAY_HEIGHT / 8;
  //         }
  //       } else {
  //         p += width * (JAVELIN_DISPLAY_HEIGHT / 8);
  //         f += width;
  //       }
  //       p += JAVELIN_DISPLAY_HEIGHT / 8;
  //     }
  //   }
  // #elif JAVELIN_DISPLAY_WIDTH >= 23
  //   constexpr size_t MAXIMUM_STROKES = JAVELIN_DISPLAY_HEIGHT;
  //   size_t startingStroke =
  //       length > MAXIMUM_STROKES ? length - MAXIMUM_STROKES : 0;
  //   size_t y = length < MAXIMUM_STROKES ? MAXIMUM_STROKES - length : 0;

  //   for (size_t i = startingStroke; i < length; ++i) {
  //     uint32_t stroke = strokes[i].GetKeyState();
  //     uint8_t *p = &buffer8[y / 8];
  //     uint8_t mask = 1 << (y & 7);
  //     ++y;

  //     for (size_t i = 0; i < 23; ++i) {
  //       if (stroke & (1 << i)) {
  //         *p |= mask;
  //       }
  //       p += JAVELIN_DISPLAY_HEIGHT / 8;
  //     }
  //   }
  // #endif
}

//---------------------------------------------------------------------------

#endif // JAVELIN_DISPLAY_DRIVER == 7789

//---------------------------------------------------------------------------
