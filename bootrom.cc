//---------------------------------------------------------------------------

#include "bootrom.h"

//---------------------------------------------------------------------------

void Bootrom::Launch() {
  const uint16_t *const functionTableAddress = (const uint16_t *)0x14;
  size_t (*LookupMethod)(uint32_t table, uint32_t key) =
      (size_t(*)(uint32_t, uint32_t))(size_t)functionTableAddress[2];
  void (*UsbBoot)(int, int) = (void (*)(int, int))LookupMethod(
      functionTableAddress[0], 'U' + 'B' * 256);
  (*UsbBoot)(0, 0);
}

//---------------------------------------------------------------------------

#if JAVELIN_SPLIT

Bootrom Bootrom::instance;

void Bootrom::UpdateBuffer(TxBuffer &buffer) {
  if (!launchSlave) {
    return;
  }
  launchSlave = false;
  buffer.Add(SplitHandlerId::BOOTROM, nullptr, 0);
}

void Bootrom::OnDataReceived(const void *data, size_t length) { Launch(); }

#endif

//---------------------------------------------------------------------------
