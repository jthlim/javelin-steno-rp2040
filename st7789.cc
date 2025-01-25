//---------------------------------------------------------------------------

#include "st7789.h"
#include "javelin/button_script_manager.h"
#include "javelin/clock.h"
#include "javelin/console.h"
#include "javelin/font/monochrome/font.h"
#include "javelin/hal/display.h"
#include "javelin/utf8_pointer.h"
#include "rp2040_dma.h"
#include <hardware/gpio.h>
#include <hardware/spi.h>
#include <pico/time.h>
#include <string.h>

//---------------------------------------------------------------------------

#if JAVELIN_DISPLAY_DRIVER == 7789

//---------------------------------------------------------------------------

enum St7789Command : uint8_t {
  SOFTWARE_RESET = 0x01,
  SLEEP_ON = 0x10,
  SLEEP_OFF = 0x11,
  SET_DISPLAY_MODE_PARTIAL = 0x12,
  SET_DISPLAY_MODE_NORMAL = 0x13,
  DISPLAY_INVERSION_OFF = 0x20,
  DISPLAY_INVERSION_ON = 0x21,
  DISPLAY_OFF = 0x28,
  DISPLAY_ON = 0x29,
  SET_COLUMN_ADDRESS = 0x2a,
  SET_ROW_ADDRESS = 0x2b,
  MEMORY_WRITE = 0x2c,
  MEMORY_DATA_ACCESS_CONTROL = 0x36,
  SET_INTERFACE_PIXEL_FORMAT = 0x3a,
  SET_DISPLAY_BRIGHTNESS = 0x51,
};

//---------------------------------------------------------------------------

#if JAVELIN_SPLIT
St7789::St7789Data St7789::instances[2];
#else
St7789::St7789Data St7789::instances[1];
#endif

//---------------------------------------------------------------------------

void St7789::SendCommand(St7789Command command, const void *data,
                         size_t length) {
  gpio_put(JAVELIN_DISPLAY_CS_PIN, 0);
  gpio_put(JAVELIN_DISPLAY_DC_PIN, 0);

  sleep_us(1);
  spi_write_blocking(JAVELIN_DISPLAY_SPI, (const uint8_t *)&command,
                     sizeof(command));

  if (length) {
    gpio_put(JAVELIN_DISPLAY_DC_PIN, 1);

    sleep_us(1);
    spi_write_blocking(JAVELIN_DISPLAY_SPI, (const uint8_t *)data, length);
  }

  gpio_put(JAVELIN_DISPLAY_CS_PIN, 1);
  gpio_put(JAVELIN_DISPLAY_DC_PIN, 1);
}

void St7789::SetLR(uint32_t left, uint32_t right) {
  const uint8_t data[] = {
      (uint8_t)(left >> 8),
      (uint8_t)(left & 0xff),
      (uint8_t)(right >> 8),
      (uint8_t)(right & 0xff),
  };

  SendCommand(St7789Command::SET_COLUMN_ADDRESS, data, sizeof(data));
}

void St7789::SetTB(uint32_t top, uint32_t bottom) {
  const uint8_t data[] = {
      (uint8_t)(top >> 8),
      (uint8_t)(top & 0xff),
      (uint8_t)(bottom >> 8),
      (uint8_t)(bottom & 0xff),
  };

  SendCommand(St7789Command::SET_ROW_ADDRESS, data, sizeof(data));
}

void St7789::St7789Data::Initialize() {
  spi_init(JAVELIN_DISPLAY_SPI, 125'000'000);
  spi_set_format(JAVELIN_DISPLAY_SPI, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);

  gpio_init(JAVELIN_DISPLAY_MISO_PIN);
  gpio_set_function(JAVELIN_DISPLAY_MISO_PIN, GPIO_FUNC_SPI);

  gpio_init(JAVELIN_DISPLAY_MOSI_PIN);
  gpio_set_function(JAVELIN_DISPLAY_MOSI_PIN, GPIO_FUNC_SPI);

  gpio_init(JAVELIN_DISPLAY_SCK_PIN);
  gpio_set_function(JAVELIN_DISPLAY_SCK_PIN, GPIO_FUNC_SPI);

  gpio_init(JAVELIN_DISPLAY_DC_PIN);
  gpio_set_dir(JAVELIN_DISPLAY_DC_PIN, GPIO_OUT);
  gpio_put(JAVELIN_DISPLAY_DC_PIN, 0); // Command = 0, Data = 1

  gpio_init(JAVELIN_DISPLAY_CS_PIN);
  gpio_set_dir(JAVELIN_DISPLAY_CS_PIN, GPIO_OUT);
  gpio_put(JAVELIN_DISPLAY_CS_PIN, 1);

  gpio_init(JAVELIN_DISPLAY_RST_PIN);
  gpio_set_dir(JAVELIN_DISPLAY_RST_PIN, GPIO_OUT);
  gpio_put(JAVELIN_DISPLAY_RST_PIN, 0);
  sleep_ms(100);
  gpio_put(JAVELIN_DISPLAY_RST_PIN, 1);
  sleep_ms(150);

  SendCommand(St7789Command::SOFTWARE_RESET);
  sleep_ms(120);

  SendCommand(St7789Command::SLEEP_OFF, NULL, 0);
  sleep_ms(5);

  static const uint8_t interfaceFormat = 0x55; // 565 rgb interface + control.
  SendCommand(St7789Command::SET_INTERFACE_PIXEL_FORMAT, &interfaceFormat, 1);
  sleep_ms(10);

  static const uint8_t memoryDataAccessControl = 0;
  SendCommand(St7789Command::MEMORY_DATA_ACCESS_CONTROL,
              &memoryDataAccessControl, 1);

  SendCommand(St7789Command::DISPLAY_INVERSION_ON);
  SendCommand(St7789Command::SET_DISPLAY_MODE_NORMAL, NULL, 0);

  dirty = true;
  Update();

  dma4->WaitUntilComplete();

  SendCommand(St7789Command::DISPLAY_ON, NULL, 0);
  sleep_ms(200);

  // available = true;
}

// bool St7789::SendCommand(uint8_t command) {
//   uint8_t buffer[2] = {0x80, command};
//   return i2c_write_blocking(JAVELIN_OLED_I2C, JAVELIN_OLED_I2C_ADDRESS,
//   buffer,
//                             2, false) == 2;
// }

// bool St7789::SendCommandList(const uint8_t *commands, size_t length) {
//   for (size_t i = 0; i < length; ++i) {
//     if (!SendCommand(commands[i])) {
//       return false;
//     }
//   }
//   return true;
// }

// void St7789::SendCommandListDma(const uint8_t *commands, size_t length) {
//   JAVELIN_OLED_I2C->hw->enable = 0;
//   JAVELIN_OLED_I2C->hw->tar = JAVELIN_OLED_I2C_ADDRESS;
//   JAVELIN_OLED_I2C->hw->enable = 1;

//   // Start of data.
//   uint16_t *d = dmaBuffer;
//   for (size_t i = 0; i < length; ++i) {
//     *d++ = 0x80;
//     *d++ = commands[i] | 0x200;
//   }

//   SendDmaBuffer(length * 2);
// }

// bool St7789::IsI2cTxReady() {
//   return (JAVELIN_OLED_I2C->hw->raw_intr_stat &
//           I2C_IC_RAW_INTR_STAT_TX_EMPTY_BITS) != 0;
// }

// void St7789::WaitForI2cTxReady() {
//   while (!IsI2cTxReady()) {
//     // Do nothing.
//   }
// }

// void St7789::SendDmaBuffer(size_t count) {
//   dma4->count = count;
//   dma4->source = dmaBuffer;
//   dma4->destination = &JAVELIN_OLED_I2C->hw->data_cmd;

//   Rp2040DmaControl dmaControl = {
//       .enable = true,
//       .dataSize = Rp2040DmaControl::DataSize::HALF_WORD,
//       .incrementRead = true,
//       .incrementWrite = false,
//       .chainToDma = 4,
//       .transferRequest = Rp2040DmaTransferRequest::I2C1_TX,
//       .sniffEnable = false,
//   };
//   dma4->controlTrigger = dmaControl;
// }

//---------------------------------------------------------------------------

void St7789::St7789Data::SetPixel(uint32_t x, uint32_t y) {
  if (x >= JAVELIN_DISPLAY_WIDTH || y >= JAVELIN_DISPLAY_HEIGHT) {
    return;
  }

  const size_t pixelIndex = y * JAVELIN_DISPLAY_WIDTH + x;
  buffer16[pixelIndex] = drawColor;
}

void St7789::St7789Data::Clear() {
  // if (!available) {
  //   return;
  // }
  dirty = true;
  Mem::Clear(buffer32);
}

void St7789::St7789Data::DrawLine(int x0, int y0, int x1, int y1) {
  // if (!available) {
  //   return;
  // }
  dirty = true;

  // Use balanced Bresenham's line algorithm:
  // https://en.wikipedia.org/wiki/Bresenham's_line_algorithm
  const int dx = abs(x1 - x0);
  const int sx = x0 < x1 ? 1 : -1;
  const int dy = -abs(y1 - y0);
  const int sy = y0 < y1 ? 1 : -1;
  int error = dx + dy;

  while (true) {
    SetPixel(x0, y0);
    if (x0 == x1 && y0 == y1) {
      return;
    }

    const int e2 = 2 * error;
    if (e2 >= dy) {
      error += dy;
      x0 += sx;
    }
    if (e2 <= dx) {
      error += dx;
      y0 += sy;
    }
  }
}

void St7789::St7789Data::DrawRect(int left, int top, int right, int bottom) {
  // if (!available) {
  //   return;
  // }

  if (right < 0 || bottom < 0 || left >= JAVELIN_DISPLAY_WIDTH ||
      top >= JAVELIN_DISPLAY_HEIGHT) {
    return;
  }
  if (left < 0) {
    left = 0;
  }
  if (right >= JAVELIN_DISPLAY_WIDTH) {
    right = JAVELIN_DISPLAY_WIDTH;
  }
  int width = right - left;

  if (top < 0) {
    top = 0;
  }
  if (bottom >= JAVELIN_DISPLAY_HEIGHT) {
    bottom = JAVELIN_DISPLAY_HEIGHT;
  }

  dirty = true;

  uint16_t *p = &buffer16[top * JAVELIN_DISPLAY_WIDTH + left];
  for (int y = top; y < bottom; y++) {
    for (int x = 0; x < width; ++x) {
      p[x] = drawColor;
    }
    p += JAVELIN_DISPLAY_WIDTH;
  }
}

void St7789::St7789Data::DrawImage(int x, int y, int width, int height,
                                   const uint8_t *data) {
  // if (!available) {
  //   return;
  // }

  // It's all off the screen.
  if (x >= JAVELIN_DISPLAY_WIDTH || y >= JAVELIN_DISPLAY_HEIGHT) {
    return;
  }

  if (y + height <= 0) {
    return;
  }

  size_t bytesPerColumn = (height + 7) >> 3;

  if (x < 0) {
    width += x;
    if (width <= 0) {
      return;
    }
    data -= bytesPerColumn * x;
    x = 0;
  }

  dirty = true;

  uint16_t *p = &buffer16[x];

  const int endX = x + width;
  if (endX > JAVELIN_DISPLAY_WIDTH) {
    width = JAVELIN_DISPLAY_WIDTH - x;
  }

  while (width > 0) {
    for (int yy = 0, topY = y; yy < bytesPerColumn; ++yy, topY += 8) {
      int v = *data++;
      while (v) {
        int nextRow = __builtin_ctz(v);
        uint32_t pixelY = topY + nextRow;
        if (pixelY < JAVELIN_DISPLAY_HEIGHT) {
          p[pixelY * JAVELIN_DISPLAY_WIDTH] = drawColor;
        }
        v &= v - 1;
      }
    }

    p++;
    --width;
  }
}

void St7789::St7789Data::DrawGrayscaleRange(int x, int y, int width, int height,
                                            const uint8_t *data, int min,
                                            int max) {
  // It's all off the screen.
  if (x >= JAVELIN_DISPLAY_WIDTH || y >= JAVELIN_DISPLAY_HEIGHT) {
    return;
  }

  if (y < 0) {
    height += y;
    if (height <= 0) {
      return;
    }
    data -= y;
    y = 0;
  }

  const size_t bytesPerColumn = height;

  if (x < 0) {
    width += x;
    if (width <= 0) {
      return;
    }
    data -= bytesPerColumn * x;
    x = 0;
  }

  dirty = true;

  // TODO
}

void St7789::St7789Data::DrawText(int x, int y, const Font *font,
                                  TextAlignment alignment, const char *text) {
  // if (!available) {
  //   return;
  // }

  Utf8Pointer utf8p(text);
  y -= font->baseline;

  switch (alignment) {
  case TextAlignment::LEFT:
    break;
  case TextAlignment::MIDDLE:
    x -= font->GetStringWidth(text) >> 1;
    break;
  case TextAlignment::RIGHT:
    x -= font->GetStringWidth(text);
    break;
  }

  for (;;) {
    const uint32_t c = *utf8p++;
    if (c == 0) {
      return;
    }

    const uint8_t *data = font->GetCharacterData(c);
    if (data) {
      uint32_t width = font->GetCharacterWidth(c);
      DrawImage(x, y, width, font->height, data);
      x += width;
    }
    x += font->spacing;
  }
}

void St7789::St7789Data::Update() {
  // if (!available) {
  //   return;
  // }

  control.Update();

  if (!dirty) { // || dma4->IsBusy() || !IsI2cTxReady()) {
    return;
  }

  ButtonScriptManager::GetInstance().ExecuteScript(
      ButtonScriptId::DISPLAY_OVERLAY);

  dirty = false;

  constexpr int xOffset = (240 - JAVELIN_DISPLAY_SCREEN_WIDTH) / 2;
  constexpr int yOffset = (320 - JAVELIN_DISPLAY_SCREEN_HEIGHT) / 2;

  SetLR(xOffset, xOffset + JAVELIN_DISPLAY_SCREEN_WIDTH - 1);
  SetTB(yOffset, yOffset + JAVELIN_DISPLAY_SCREEN_HEIGHT - 1);
  SendCommand(St7789Command::MEMORY_WRITE);

  gpio_put(JAVELIN_DISPLAY_CS_PIN, 0);
  gpio_put(JAVELIN_DISPLAY_DC_PIN, 1);
  sleep_us(1);

  dma4->source = buffer32;
  dma4->destination = &spi_get_hw(JAVELIN_DISPLAY_SPI)->dr;
  dma4->count = sizeof(buffer32);

  constexpr Rp2040DmaControl dmaControl = {
      .enable = true,
      .dataSize = Rp2040DmaControl::DataSize::BYTE,
      .incrementRead = true,
      .incrementWrite = false,
      .chainToDma = 4,
      .transferRequest = Rp2040DmaTransferRequest::SPI1_TX,
      .sniffEnable = false,
  };
  dma4->controlTrigger = dmaControl;
}

//---------------------------------------------------------------------------

void St7789::St7789Control::Update() {
  if (dirtyFlag == 0 || dma4->IsBusy()) {
    return;
  }

  if (dirtyFlag & DIRTY_FLAG_SCREEN_ON) {
    SendCommand(data.screenOn ? St7789Command::DISPLAY_ON
                              : St7789Command::DISPLAY_OFF);
  }

  if (dirtyFlag & DIRTY_FLAG_CONTRAST) {
    SendCommand(St7789Command::SET_DISPLAY_BRIGHTNESS, &data.contrast, 1);
  }

  dirtyFlag = 0;
}

//---------------------------------------------------------------------------

// void St7789::St7789Availability::UpdateBuffer(TxBuffer &buffer) {
//   if (!dirty) {
//     return;
//   }
//   dirty = false;
//   buffer.Add(SplitHandlerId::DISPLAY_AVAILABLE, &available, sizeof(bool));
// }

// void St7789::St7789Availability::OnDataReceived(const void *data,
//                                                 size_t length) {
//   available = *(bool *)data;
// }

void St7789::St7789Control::UpdateBuffer(TxBuffer &buffer) {
  if (dirtyFlag == 0) {
    return;
  }
  dirtyFlag = 0;
  buffer.Add(SplitHandlerId::DISPLAY_CONTROL, &data, sizeof(data));
}

void St7789::St7789Control::OnDataReceived(const void *data, size_t length) {
  const St7789ControlTxRxData *controlData =
      (const St7789ControlTxRxData *)data;

  if (this->data.screenOn != controlData->screenOn) {
    this->data.screenOn = controlData->screenOn;
    dirtyFlag |= DIRTY_FLAG_SCREEN_ON;
  }

  if (this->data.contrast != controlData->contrast) {
    this->data.contrast = controlData->contrast;
    dirtyFlag |= DIRTY_FLAG_CONTRAST;
  }
}

void St7789::St7789Control::OnTransmitConnectionReset() {
  dirtyFlag = DIRTY_FLAG_SCREEN_ON | DIRTY_FLAG_CONTRAST;
}

struct St7789TxRxData {
  uint32_t offset;
  uint32_t size;
  uint32_t data[7296 / 4];
};

void St7789::St7789Data::UpdateBuffer(TxBuffer &buffer) {
  // if (!available) {
  //   return;
  // }
  if (!dirty && txRxOffset == 0) {
    return;
  }
  if (dma4->IsBusy()) {
    return;
  }

  St7789TxRxData *txData =
      (St7789TxRxData *)buffer.Reserve(sizeof(St7789TxRxData));
  if (txData == nullptr) {
    return;
  }

  if (dirty && txRxOffset == 0) {
    dirty = false;
  }

  txData->offset = txRxOffset;
  txRxOffset += sizeof(St7789TxRxData::data);
  if (txRxOffset > sizeof(buffer32)) {
    txData->size = sizeof(buffer32) - txData->offset;
    txRxOffset = 0;
  } else {
    txData->size = sizeof(St7789TxRxData::data);
  }
  dma6->source = buffer32 + (txData->offset / 4);
  dma6->destination = txData->data;
  dma6->count = txData->size / 4;
  constexpr Rp2040DmaControl dmaControl = {
      .enable = true,
      .dataSize = Rp2040DmaControl::DataSize::WORD,
      .incrementRead = true,
      .incrementWrite = true,
      .chainToDma = 6,
      .transferRequest = Rp2040DmaTransferRequest::PERMANENT,
      .sniffEnable = false,
  };
  dma6->controlTrigger = dmaControl;
  dma6->WaitUntilComplete();

  buffer.Add(SplitHandlerId::DISPLAY_DATA, 8 + txData->size);
}

void St7789::St7789Data::OnDataReceived(const void *data, size_t length) {
  St7789TxRxData *rxData = (St7789TxRxData *)data;

  // Just in case corrupt data comes along
  if (rxData->offset > sizeof(buffer32) ||
      rxData->offset + rxData->size > sizeof(buffer32)) {
    return;
  }

  dma6->source = rxData->data;
  dma6->destination = buffer32 + (rxData->offset / 4);
  dma6->count = rxData->size / 4;
  constexpr Rp2040DmaControl dmaControl = {
      .enable = true,
      .dataSize = Rp2040DmaControl::DataSize::WORD,
      .incrementRead = true,
      .incrementWrite = true,
      .chainToDma = 6,
      .transferRequest = Rp2040DmaTransferRequest::PERMANENT,
      .sniffEnable = false,
  };
  dma6->controlTrigger = dmaControl;
  dma6->WaitUntilComplete();

  if (rxData->offset + rxData->size >= sizeof(buffer32)) {
    dirty = true;
  }
}

//---------------------------------------------------------------------------

void Display::Clear(int displayId) {
#if JAVELIN_SPLIT
  if (displayId < 0 || displayId >= 2) {
    return;
  }
#else
  displayId = 0;
#endif
  St7789::instances[displayId].Clear();
}

void Display::SetScreenOn(int displayId, bool on) {
#if JAVELIN_SPLIT
  if (displayId < 0 || displayId >= 2) {
    return;
  }
#else
  displayId = 0;
#endif
  St7789::instances[displayId].control.SetScreenOn(on);
}

void Display::SetContrast(int displayId, int contrast) {
#if JAVELIN_SPLIT
  if (displayId < 0 || displayId >= 2) {
    return;
  }
#else
  displayId = 0;
#endif
  St7789::instances[displayId].control.SetContrast(contrast > 255 ? 255
                                                                  : contrast);
}

void Display::DrawPixel(int displayId, int x, int y) {
#if JAVELIN_SPLIT
  if (displayId < 0 || displayId >= 2) {
    return;
  }
#else
  displayId = 0;
#endif

  St7789::instances[displayId].dirty = true;
  St7789::instances[displayId].SetPixel(x, y);
}

void Display::DrawLine(int displayId, int x1, int y1, int x2, int y2) {
#if JAVELIN_SPLIT
  if (displayId < 0 || displayId >= 2) {
    return;
  }
#else
  displayId = 0;
#endif

  St7789::instances[displayId].DrawLine(x1, y1, x2, y2);
}

void Display::DrawImage(int displayId, int x, int y, int width, int height,
                        const uint8_t *data) {
#if JAVELIN_SPLIT
  if (displayId < 0 || displayId >= 2) {
    return;
  }
#else
  displayId = 0;
#endif

  St7789::instances[displayId].DrawImage(x, y, width, height, data);
}

void Display::DrawGrayscaleRange(int displayId, int x, int y, int width,
                                 int height, const uint8_t *data, int min,
                                 int max) {
#if JAVELIN_SPLIT
  if (displayId < 0 || displayId >= 2) {
    return;
  }
#else
  displayId = 0;
#endif

  St7789::instances[displayId].DrawGrayscaleRange(x, y, width, height, data,
                                                  min, max);
}

void Display::DrawText(int displayId, int x, int y, FontId fontId,
                       TextAlignment alignment, const char *text) {
#if JAVELIN_SPLIT
  if (displayId < 0 || displayId >= 2) {
    return;
  }
#else
  displayId = 0;
#endif

  const Font *font = Font::GetFont(fontId);
  St7789::instances[displayId].DrawText(x, y, font, alignment, text);
}

void Display::DrawRect(int displayId, int left, int top, int right,
                       int bottom) {
#if JAVELIN_SPLIT
  if (displayId < 0 || displayId >= 2) {
    return;
  }
#else
  displayId = 0;
#endif
  St7789::instances[displayId].DrawRect(left, top, right, bottom);
}

void Display::SetDrawColor(int displayId, int color) {
#if JAVELIN_SPLIT
  if (displayId < 0 || displayId >= 2) {
    return;
  }
#else
  displayId = 0;
#endif
  int r = (color >> 19) & 0x1f;
  int g = (color >> 10) & 0x3f;
  int b = (color >> 3) & 0x1f;

  int rgb565 = (r << 11) | (g << 5) | b;
  int pixelColor;
  asm("rev16 %0, %1" : "=r"(pixelColor) : "r"(rgb565));

  St7789::instances[displayId].drawColor = pixelColor;
}

void Display::SetDrawColorRgb(int displayId, int r, int g, int b) {
#if JAVELIN_SPLIT
  if (displayId < 0 || displayId >= 2) {
    return;
  }
#else
  displayId = 0;
#endif
  r = (r >> 3) & 0x1f;
  g = (g >> 2) & 0x3f;
  b = (b >> 3) & 0x1f;

  int rgb565 = (r << 11) | (g << 5) | b;
  int pixelColor;
  asm("rev16 %0, %1" : "=r"(pixelColor) : "r"(rgb565));

  St7789::instances[displayId].drawColor = pixelColor;
}

//---------------------------------------------------------------------------

#endif // JAVELIN_DISPLAY_DRIVER == 7789

//---------------------------------------------------------------------------
