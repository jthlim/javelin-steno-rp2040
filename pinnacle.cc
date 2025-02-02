//---------------------------------------------------------------------------

#include "pinnacle.h"
#include "javelin/button_script_manager.h"
#include "javelin/clock.h"

#include <hardware/gpio.h>
#include <hardware/spi.h>

//---------------------------------------------------------------------------

#if JAVELIN_POINTER == 0x73a

//---------------------------------------------------------------------------

enum class PinnacleRegister : int {
  FIRMWARE_ID,
  FIMRWARE_VERSION,
  STATUS,
  SYSTEM_CONFIG,
  FEED_CONFIG_1,
  FEED_CONFIG_2,
  FEED_CONFIG_3,
  CALIBRATION_CONFIG,
  PS2_AUX_CONTROL,
  SAMPLE_RATE,
  Z_IDLE,
  Z_SCALER,
  SLEEP_INTERVAL,
  SLEEP_TIMER,
  EMI_ADJUST,
  _RESERVED_0F,
  _RESERVED_10,
  _RESERVED_11,
  PACKET_BYTE_0,
  PACKET_BYTE_1,
  PACKET_BYTE_2,
  PACKET_BYTE_3,
  PACKET_BYTE_4,
  PACKET_BYTE_5,
  PORT_A_GPIO_CONTROL,
  PORT_A_GPIO_DATA,
  PORT_B_GPIO_CONTROL_AND_DATA,
  EXTENDED_REGISTER_VALUE,
  EXTENDED_REGISTER_ADDRESS_HIGH,
  EXTENDED_REGISTER_ADDRESS_LOW,
  EXTENDED_REGISTER_CONTROL,
  PRODUCT_ID,

  // Taken from Cirque example code:
  // https://github.com/cirque-corp/Cirque_Pinnacle_1CA027
  //
  // File:
  // Circular_Trackpad/Single_Pad_Sample_Code/SPI_CurvedOverlay/SPI_CurvedOverlay.ino
  X_AXIS_WIDE_Z_MIN = 0x149,
  Y_AXIS_WIDE_Z_MIN = 0x168,
  ADC_ATTENUATION = 0x187,
};

//---------------------------------------------------------------------------

Pinnacle Pinnacle::instance;

//---------------------------------------------------------------------------

void Pinnacle::Initialize() {
  spi_init(JAVELIN_POINTER_SPI, 12'500'000);
  spi_set_format(JAVELIN_POINTER_SPI, 8, SPI_CPOL_0, SPI_CPHA_1, SPI_MSB_FIRST);

  gpio_init(JAVELIN_POINTER_MISO_PIN);
  gpio_set_function(JAVELIN_POINTER_MISO_PIN, GPIO_FUNC_SPI);

  gpio_init(JAVELIN_POINTER_MOSI_PIN);
  gpio_set_function(JAVELIN_POINTER_MOSI_PIN, GPIO_FUNC_SPI);

  gpio_init(JAVELIN_POINTER_SCK_PIN);
  gpio_set_function(JAVELIN_POINTER_SCK_PIN, GPIO_FUNC_SPI);

  gpio_init(JAVELIN_POINTER_CS_PIN);
  gpio_set_dir(JAVELIN_POINTER_CS_PIN, GPIO_OUT);
  gpio_put(JAVELIN_POINTER_CS_PIN, 1);

  uint8_t id[2];
  ReadRegisters(PinnacleRegister::FIRMWARE_ID, id, 2);
  if (id[0] != 0x7 || id[1] != 0x3a) {
    Uninitialize();
    instance.available = false;
    return;
  }

  ClearFlags();
  WriteRegister(PinnacleRegister::SYSTEM_CONFIG, 0);

  WriteRegister(PinnacleRegister::X_AXIS_WIDE_Z_MIN, 4);
  WriteRegister(PinnacleRegister::Y_AXIS_WIDE_Z_MIN, 3);
  int adcAttenuation = ReadRegister(PinnacleRegister::ADC_ATTENUATION);
  WriteRegister(PinnacleRegister::ADC_ATTENUATION, adcAttenuation & 0x3f);

  // Disable GlideExtend, Scroll, Secondary tap, All Taps, Intellimouse
  WriteRegister(PinnacleRegister::FEED_CONFIG_2, 0x1f);

  // Feed enable, absolute mode.
  constexpr int feedConfig = 3 // Feed enable, absolute mode
#if JAVELIN_POINTER_INVERT_X
                             | 0x40
#endif
#if JAVELIN_POINTER_INVERT_Y
                             | 0x80
#endif
      ;
  WriteRegister(PinnacleRegister::FEED_CONFIG_1, feedConfig);

  WriteRegister(PinnacleRegister::Z_IDLE, 2);

  instance.available = true;
}

void Pinnacle::Uninitialize() {
  spi_deinit(JAVELIN_POINTER_SPI);
  gpio_init(JAVELIN_POINTER_MISO_PIN);
  gpio_init(JAVELIN_POINTER_MOSI_PIN);
  gpio_init(JAVELIN_POINTER_SCK_PIN);
  gpio_init(JAVELIN_POINTER_CS_PIN);
}

//---------------------------------------------------------------------------

void Pinnacle::WriteRegister(PinnacleRegister reg, int value) {
  gpio_put(JAVELIN_POINTER_CS_PIN, 0);
  if ((int)reg > 0x20) {
    WriteRegister(PinnacleRegister::EXTENDED_REGISTER_VALUE, value);
    WriteRegister(PinnacleRegister::EXTENDED_REGISTER_ADDRESS_HIGH,
                  int(reg) >> 8);
    WriteRegister(PinnacleRegister::EXTENDED_REGISTER_ADDRESS_LOW,
                  int(reg) & 0xff);
    WriteRegister(PinnacleRegister::EXTENDED_REGISTER_CONTROL, 2);

    while (ReadRegister(PinnacleRegister::EXTENDED_REGISTER_CONTROL) != 0) {
      // Busy wait...
    }

    ClearFlags();
  } else {
    const uint8_t writeData[2] = {uint8_t(0x80 | (int)reg), uint8_t(value)};
    spi_write_blocking(JAVELIN_POINTER_SPI, writeData, 2);
  }
  gpio_put(JAVELIN_POINTER_CS_PIN, 1);
}

int Pinnacle::ReadRegister(PinnacleRegister reg) {
  uint8_t result;

  if ((int)reg > 0x20) {
    WriteRegister(PinnacleRegister::EXTENDED_REGISTER_ADDRESS_HIGH,
                  int(reg) >> 8);
    WriteRegister(PinnacleRegister::EXTENDED_REGISTER_ADDRESS_LOW,
                  int(reg) & 0xff);
    WriteRegister(PinnacleRegister::EXTENDED_REGISTER_CONTROL, 1);

    while (ReadRegister(PinnacleRegister::EXTENDED_REGISTER_CONTROL) != 0) {
      // Busy wait...
    }

    result = ReadRegister(PinnacleRegister::EXTENDED_REGISTER_VALUE);
    ClearFlags();
  } else {
    gpio_put(JAVELIN_POINTER_CS_PIN, 0);
    const uint8_t writeData[3] = {uint8_t(0xa0 | (int)reg), 0xfb, 0xfb};
    spi_write_blocking(JAVELIN_POINTER_SPI, writeData, 3);

    spi_read_blocking(JAVELIN_POINTER_SPI, 0xfb, &result, 1);
    gpio_put(JAVELIN_POINTER_CS_PIN, 1);
  }

  return result;
}

void Pinnacle::ReadRegisters(PinnacleRegister startingReg, uint8_t *result,
                             size_t length) {
  gpio_put(JAVELIN_POINTER_CS_PIN, 0);
  const uint8_t writeData[3] = {uint8_t(0xa0 | (int)startingReg), 0xfc, 0xfc};
  spi_write_blocking(JAVELIN_POINTER_SPI, writeData, 3);

  spi_read_blocking(JAVELIN_POINTER_SPI, 0xfc, result, length);

  gpio_put(JAVELIN_POINTER_CS_PIN, 1);
}

void Pinnacle::ClearFlags() { WriteRegister(PinnacleRegister::STATUS, 0); }

//---------------------------------------------------------------------------

void Pinnacle::UpdateInternal() {
  if (available) {
    // Check for data ready
    const int status = ReadRegister(PinnacleRegister::STATUS);
    if ((status & 4) != 0) {
      uint8_t registerData[4];
      ReadRegisters(PinnacleRegister::PACKET_BYTE_2, registerData, 4);
      ClearFlags();

      Pointer newData;
      newData.x = registerData[0] + ((registerData[2] & 0xf) << 8);
      newData.y = registerData[1] + ((registerData[2] & 0xf0) << 4);
      newData.z = registerData[3] & 0x1f;

      if (newData != data[JAVELIN_POINTER_LOCAL_OFFSET].pointer) {
        data[JAVELIN_POINTER_LOCAL_OFFSET].isDirty = true;
        data[JAVELIN_POINTER_LOCAL_OFFSET].pointer = newData;
      }
    }
  }

  for (size_t i = 0; i < JAVELIN_POINTER_COUNT; ++i) {
    if (data[i].isDirty) {
      data[i].isDirty = false;
      CallScript(i, data[i].pointer);
    }
  }
}

//---------------------------------------------------------------------------

#if JAVELIN_SPLIT && JAVELIN_SPLIT_IS_MASTER
void Pinnacle::OnDataReceived(const void *data, size_t length) {
  const Pointer *pointer = (const Pointer *)data;
  this->data[JAVELIN_POINTER_LOCAL_OFFSET + 1].isDirty = true;
  this->data[JAVELIN_POINTER_LOCAL_OFFSET + 1].pointer = *pointer;
}
#endif

#if JAVELIN_SPLIT && !JAVELIN_SPLIT_IS_MASTER
void Pinnacle::UpdateBuffer(TxBuffer &buffer) {
  if (!data[JAVELIN_POINTER_LOCAL_OFFSET].isDirty) {
    return;
  }

  if (buffer.Add(SplitHandlerId::POINTER,
                 &data[JAVELIN_POINTER_LOCAL_OFFSET].pointer,
                 sizeof(Pointer))) {
    data[JAVELIN_POINTER_LOCAL_OFFSET].isDirty = false;
  }
}
#endif

//---------------------------------------------------------------------------

void Pinnacle::CallScript(size_t pointerIndex, const Pointer &pointer) {
#if !defined(JAVELIN_ENCODER_COUNT)
#define JAVELIN_ENCODER_COUNT 0
#endif

  constexpr size_t POINTER_SCRIPT_OFFSET =
      (2 + 2 * BUTTON_COUNT + 2 * JAVELIN_ENCODER_COUNT);
  const size_t scriptIndex = POINTER_SCRIPT_OFFSET + pointerIndex;

  const intptr_t parameters[3] = {pointer.x, pointer.y, pointer.z};

  ButtonScriptManager::GetInstance().ExecuteScriptIndex(
      scriptIndex, Clock::GetMilliseconds(), parameters, 3);
}

//---------------------------------------------------------------------------

#endif

//---------------------------------------------------------------------------
