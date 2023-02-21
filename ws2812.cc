//---------------------------------------------------------------------------

#include "ws2812.h"
#include "javelin/pixel.h"
#include "rp2040_dma.h"
#include "ws2812.pio.h"
#include <hardware/clocks.h>
#include <hardware/pio.h>
#include <hardware/timer.h>

//---------------------------------------------------------------------------

#if defined(JAVELIN_RGB_COUNT) && defined(JAVELIN_RGB_PIN)

//---------------------------------------------------------------------------

const PIO PIO_INSTANCE = pio0;
const int STATE_MACHINE_INDEX = 0;

//---------------------------------------------------------------------------

Ws2812::Ws28128Data Ws2812::instance;

//---------------------------------------------------------------------------

void Ws2812::Initialize() {
  const int CYCLES_PER_BIT = 10;
  const float FREQUENCY = 800000.0;

  uint offset = pio_add_program(PIO_INSTANCE, &ws2812_program);

  pio_gpio_init(PIO_INSTANCE, JAVELIN_RGB_PIN);
  pio_sm_set_consecutive_pindirs(PIO_INSTANCE, STATE_MACHINE_INDEX,
                                 JAVELIN_RGB_PIN, 1, true);
  pio_sm_config config = ws2812_program_get_default_config(offset);
  sm_config_set_sideset_pins(&config, JAVELIN_RGB_PIN);
  sm_config_set_out_pins(&config, JAVELIN_RGB_PIN, 1);
  sm_config_set_out_shift(&config, false, true, 24);
  sm_config_set_fifo_join(&config, PIO_FIFO_JOIN_TX);
  float div = clock_get_hz(clk_sys) * (1.0f / (FREQUENCY * CYCLES_PER_BIT));
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

  // clang-format off
#if JAVELIN_SPLIT
  #define JAVELIN_RGB_SLAVE_COUNT (JAVELIN_RGB_COUNT - JAVELIN_RGB_MASTER_COUNT)
  #if JAVELIN_RGB_COUNT == 2 * JAVELIN_RGB_MASTER_COUNT
    dma1->count = JAVELIN_RGB_MASTER_COUNT;
  #else
    dma1->count = IsMaster() ? JAVELIN_RGB_MASTER_COUNT
                           : JAVELIN_RGB_SLAVE_COUNT;
  #endif // JAVELIN_RGB_COUNT == 2 * JAVELIN_RGB_MASTER_COUNT
#else
  dma1->count = JAVELIN_RGB_COUNT;
#endif // JAVELIN_SPLIT
  // clang-format on
}

void Ws2812::Ws28128Data::Update() {
  if (!dirty) {
    return;
  }

  // Don't update more than once every 10ms.
  uint32_t now = time_us_32();
  uint32_t timeSinceLastUpdate = now - lastUpdate;
  if (timeSinceLastUpdate < 10000) {
    return;
  }

  dirty = false;
  lastUpdate = now;

#if JAVELIN_SPLIT
  dma1->sourceTrigger = SplitTxRx::IsMaster()
                            ? pixelValues
                            : pixelValues + JAVELIN_RGB_MASTER_COUNT;
#else
  dma1->sourceTrigger = &pixelValues;
#endif
}

#if JAVELIN_SPLIT
void Ws2812::Ws28128Data::UpdateBuffer(TxBuffer &buffer) {
  if (!slaveDirty) {
    return;
  }

  slaveDirty = false;
  buffer.Add(SplitHandlerId::RGB, pixelValues + JAVELIN_RGB_MASTER_COUNT,
             sizeof(uint32_t) * JAVELIN_RGB_SLAVE_COUNT);
}

void Ws2812::Ws28128Data::OnDataReceived(const void *data, size_t length) {
  if (memcmp(data, pixelValues + JAVELIN_RGB_MASTER_COUNT,
             sizeof(uint32_t) * JAVELIN_RGB_SLAVE_COUNT) == 0) {
    return;
  }
  dirty = true;
  memcpy(pixelValues + JAVELIN_RGB_MASTER_COUNT, data,
         sizeof(uint32_t) * JAVELIN_RGB_SLAVE_COUNT);
}

void Ws2812::Ws28128Data::OnConnectionReset() { slaveDirty = true; }

#endif

//---------------------------------------------------------------------------

void Pixel::SetPixel(size_t id, int r, int g, int b) {
  Ws2812::SetPixel(id, r, g, b);
}

size_t Pixel::GetCount() { return JAVELIN_RGB_COUNT; }

//---------------------------------------------------------------------------

#endif // defined(JAVELIN_RGB_COUNT) && defined(JAVELIN_RGB_PIN)

//---------------------------------------------------------------------------
