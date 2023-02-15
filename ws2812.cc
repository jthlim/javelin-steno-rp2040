//---------------------------------------------------------------------------

#include "ws2812.h"
#include "javelin/pixel.h"
#include "rp2040_dma.h"
#include <hardware/clocks.h>
#include <hardware/pio.h>
#include <hardware/timer.h>

//---------------------------------------------------------------------------

#if defined(JAVELIN_RGB_COUNT) && defined(JAVELIN_RGB_PIN)

//---------------------------------------------------------------------------

Ws2812::Ws28128Data Ws2812::instance;

//---------------------------------------------------------------------------

const int WS2812_WRAP_TARGET = 0;
const int WS2812_WRAP = 3;

const int WS2812_T1 = 2;
const int WS2812_T2 = 5;
const int WS2812_T3 = 3;

const PIO PIO_INSTANCE = pio0;
const int STATE_MACHINE_INDEX = 0;

//---------------------------------------------------------------------------

// clang-format off
static const uint16_t PROGRAM_INSTRUCTIONS[] = {
            //     .wrap_target
    0x6221, //  0: out    x, 1            side 0 [2]
    0x1123, //  1: jmp    !x, 3           side 1 [1]
    0x1400, //  2: jmp    0               side 1 [4]
    0xa442, //  3: nop                    side 0 [4]
            //     .wrap
};
// clang-format on

static const struct pio_program PROGRAM = {
    .instructions = PROGRAM_INSTRUCTIONS,
    .length = 4,
    .origin = -1,
};

//---------------------------------------------------------------------------

static inline pio_sm_config ws2812_program_get_default_config(uint offset) {
  pio_sm_config config = pio_get_default_sm_config();
  sm_config_set_wrap(&config, offset + WS2812_WRAP_TARGET,
                     offset + WS2812_WRAP);
  sm_config_set_sideset(&config, 1, false, false);
  return config;
}

void Ws2812::Initialize() {
  uint offset = pio_add_program(PIO_INSTANCE, &PROGRAM);

  pio_gpio_init(PIO_INSTANCE, JAVELIN_RGB_PIN);
  pio_sm_set_consecutive_pindirs(PIO_INSTANCE, STATE_MACHINE_INDEX,
                                 JAVELIN_RGB_PIN, 1, true);
  pio_sm_config config = ws2812_program_get_default_config(offset);
  sm_config_set_sideset_pins(&config, JAVELIN_RGB_PIN);
  sm_config_set_out_shift(&config, false, true, 24);
  sm_config_set_fifo_join(&config, PIO_FIFO_JOIN_TX);
  int cycles_per_bit = WS2812_T1 + WS2812_T2 + WS2812_T3;
  const float frequency = 800000;
  float div = clock_get_hz(clk_sys) / (frequency * cycles_per_bit);
  sm_config_set_clkdiv(&config, div);
  pio_sm_init(PIO_INSTANCE, STATE_MACHINE_INDEX, offset, &config);
  pio_sm_set_enabled(PIO_INSTANCE, STATE_MACHINE_INDEX, true);

  dma1->destination = &PIO_INSTANCE->txf[STATE_MACHINE_INDEX];

  Rp2040DmaControl dmaControl = {
      .enable = true,
      .dataSize = Rp2040DmaControl::DataSize::WORD,
      .incrementRead = true,
      .incrementWrite = false,
      .chainToDma = 1,
      .transferRequest = Rp2040DmaControl::TransferRequest::PIO0_TX0,
      .sniffEnable = false,
  };
  dma1->control = dmaControl;
  dma1->count = JAVELIN_RGB_COUNT;
}

void Ws2812::Ws28128Data::Update() {
  if (!dirty) {
    return;
  }

  // Don't update more than once every 1ms.
  uint32_t now = time_us_32();
  uint32_t timeSinceLastUpdate = now - lastUpdate;
  if (timeSinceLastUpdate < 1000) {
    return;
  }

  dirty = false;
  lastUpdate = now;

  dma1->sourceTrigger = &pixelValues;
}

void Pixel::SetPixel(size_t id, int r, int g, int b) {
  Ws2812::SetPixel(id, r, g, b);
}

//---------------------------------------------------------------------------

#endif // defined(JAVELIN_RGB_COUNT) && defined(JAVELIN_RGB_PIN)

//---------------------------------------------------------------------------
