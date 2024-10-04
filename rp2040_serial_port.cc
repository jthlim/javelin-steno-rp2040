//---------------------------------------------------------------------------

#include "javelin/hal/serial_port.h"
#include "javelin/split/split_serial_buffer.h"
#include <tusb.h>

//---------------------------------------------------------------------------

#if JAVELIN_SPLIT

void SplitSerialBuffer::SplitSerialBufferData::OnDataReceived(const void *data,
                                                              size_t length) {
  if (tud_cdc_connected()) {
    tud_cdc_write(data, length);
    tud_cdc_write_flush();
  }
}

#endif

//---------------------------------------------------------------------------

void SerialPort::SendData(const uint8_t *data, size_t length) {
  if (tud_cdc_connected()) {
    tud_cdc_write(data, length);
    tud_cdc_write_flush();
  } else {
#if JAVELIN_SPLIT
    if (Split::IsMaster()) {
      SplitSerialBuffer::Add(data, length);
    }
#endif
  }
}

//---------------------------------------------------------------------------
