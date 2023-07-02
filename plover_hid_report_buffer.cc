//---------------------------------------------------------------------------

#include "plover_hid_report_buffer.h"
#include "javelin/processor/plover_hid.h"
#include "usb_descriptors.h"

//---------------------------------------------------------------------------

PloverHidReportBuffer PloverHidReportBuffer::instance;

//---------------------------------------------------------------------------

PloverHidReportBuffer::PloverHidReportBuffer()
    : HidReportBuffer<8>(ITF_NUM_PLOVER_HID) {}

void StenoPloverHid::SendPacket(const StenoPloverHidPacket &packet) {
  PloverHidReportBuffer::instance.SendReport(
      PLOVER_HID_REPORT_ID, (uint8_t *)&packet, sizeof(packet));
}

//---------------------------------------------------------------------------
