//---------------------------------------------------------------------------

#include "split_hid_report_buffer.h"
#include "console_buffer.h"
#include "hid_keyboard_report_builder.h"
#include "javelin/console.h"
#include "plover_hid_report_buffer.h"
#include "usb_descriptors.h"
#include <pico/time.h>

//---------------------------------------------------------------------------

#if JAVELIN_SPLIT

//---------------------------------------------------------------------------

SplitHidReportBuffer::SplitHidReportBufferData SplitHidReportBuffer::instance;

//---------------------------------------------------------------------------

void SplitHidReportBuffer::SplitHidReportBufferSize::UpdateBuffer(
    TxBuffer &buffer) {
  HidBufferSize newBufferSize;
  newBufferSize.value = 0;
  newBufferSize.available[ITF_NUM_KEYBOARD] =
      HidKeyboardReportBuilder::instance.GetAvailableBufferCount();
  newBufferSize.available[ITF_NUM_CONSOLE] =
      ConsoleBuffer::instance.GetAvailableBufferCount();
  newBufferSize.available[ITF_NUM_PLOVER_HID] =
      PloverHidReportBuffer::instance.GetAvailableBufferCount();

  if (!dirty && newBufferSize.value == bufferSize.value) {
    return;
  }
  dirty = false;
  bufferSize = newBufferSize;

  buffer.Add(SplitHandlerId::HID_BUFFER_SIZE, &bufferSize, sizeof(bufferSize));
}

void SplitHidReportBuffer::SplitHidReportBufferSize::OnDataReceived(
    const void *data, size_t length) {
  bufferSize = *(HidBufferSize *)data;
}

//---------------------------------------------------------------------------

QueueEntry<SplitHidReportBuffer::EntryData> *
SplitHidReportBuffer::SplitHidReportBufferData::CreateEntry(uint8_t interface,
                                                            uint8_t reportId,
                                                            const uint8_t *data,
                                                            size_t length) {
  QueueEntry<EntryData> *entry =
      (QueueEntry<EntryData> *)malloc(sizeof(QueueEntry<EntryData>) + length);
  entry->data.interface = interface;
  entry->data.reportId = reportId;
  entry->data.length = length;
  entry->next = nullptr;
  memcpy(entry->data.data, data, length);
  return entry;
}

//---------------------------------------------------------------------------

void SplitHidReportBuffer::SplitHidReportBufferData::Add(uint8_t interface,
                                                         uint8_t reportId,
                                                         const uint8_t *data,
                                                         size_t length) {
  while (bufferSize.bufferSize.available[interface] == 0) {
    SplitTxRx::Update();
    sleep_us(100);
  }
  bufferSize.bufferSize.available[interface]--;

  QueueEntry<EntryData> *entry = CreateEntry(interface, reportId, data, length);
  AddEntry(entry);
}

void SplitHidReportBuffer::SplitHidReportBufferData::Update() {
  while (head) {
    QueueEntry<EntryData> *entry = head;

    if (!ProcessEntry(entry)) {
      return;
    }

    head = entry->next;
    if (head == nullptr) {
      tail = &head;
    }
    free(entry);
  }
}

bool SplitHidReportBuffer::SplitHidReportBufferData::ProcessEntry(
    const QueueEntry<EntryData> *entry) {
  switch (entry->data.interface) {
  case ITF_NUM_KEYBOARD: {
    auto &reportBuffer = HidKeyboardReportBuilder::instance.reportBuffer;
    if (reportBuffer.IsFull()) {
      return false;
    }

    reportBuffer.SendReport(entry->data.interface, entry->data.reportId,
                            entry->data.data, entry->data.length);
    return true;
  }
  case ITF_NUM_CONSOLE: {
    auto &reportBuffer = ConsoleBuffer::instance.reportBuffer;
    if (reportBuffer.IsFull()) {
      return false;
    }

    reportBuffer.SendReport(entry->data.interface, entry->data.reportId,
                            entry->data.data, entry->data.length);
    return true;
  }
  case ITF_NUM_PLOVER_HID: {
    auto &reportBuffer = PloverHidReportBuffer::instance;
    if (reportBuffer.IsFull()) {
      return false;
    }

    reportBuffer.SendReport(entry->data.interface, entry->data.reportId,
                            entry->data.data, entry->data.length);
    return true;
  }
  }
  // If the interface is unknown, just return true to remove it from the
  // queue.
  return true;
}

//---------------------------------------------------------------------------

void SplitHidReportBuffer::SplitHidReportBufferData::UpdateBuffer(
    TxBuffer &buffer) {
  while (head) {
    QueueEntry<EntryData> *entry = head;

    if (!buffer.Add(SplitHandlerId::HID_REPORT, &entry->data,
                    entry->data.length + sizeof(EntryData))) {
      return;
    }

    head = entry->next;
    if (head == nullptr) {
      tail = &head;
    }
    free(entry);
  }
}

void SplitHidReportBuffer::SplitHidReportBufferData::OnDataReceived(
    const void *data, size_t length) {
  const EntryData *entryData = (const EntryData *)data;

  QueueEntry<EntryData> *entry =
      CreateEntry(entryData->interface, entryData->reportId, entryData->data,
                  entryData->length);
  AddEntry(entry);
}

//---------------------------------------------------------------------------

#endif // JAVELIN_SPLIT

//---------------------------------------------------------------------------
