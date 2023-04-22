//---------------------------------------------------------------------------

#include "javelin/serial_port.h"
#include "split_serial_buffer.h"
#include "usb_descriptors.h"
#include <tusb.h>

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
