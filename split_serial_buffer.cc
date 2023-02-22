//---------------------------------------------------------------------------

#include "split_serial_buffer.h"
#include "usb_descriptors.h"
#include <tusb.h>

//---------------------------------------------------------------------------

#if JAVELIN_SPLIT

//---------------------------------------------------------------------------

SplitSerialBuffer::SplitSerialBufferData SplitSerialBuffer::instance;

//---------------------------------------------------------------------------

QueueEntry<SplitSerialBuffer::EntryData> *
SplitSerialBuffer::SplitSerialBufferData::CreateEntry(const uint8_t *data,
                                                      size_t length) {
  QueueEntry<EntryData> *entry =
      (QueueEntry<EntryData> *)malloc(sizeof(QueueEntry<EntryData>) + length);
  entry->data.length = length;
  entry->next = nullptr;
  memcpy(entry->data.data, data, length);
  return entry;
}

//---------------------------------------------------------------------------

void SplitSerialBuffer::SplitSerialBufferData::Add(const uint8_t *data,
                                                   size_t length) {
  QueueEntry<EntryData> *entry = CreateEntry(data, length);
  AddEntry(entry);
}

//---------------------------------------------------------------------------

void SplitSerialBuffer::SplitSerialBufferData::UpdateBuffer(TxBuffer &buffer) {
  while (head) {
    QueueEntry<EntryData> *entry = head;

    if (!buffer.Add(SplitHandlerId::SERIAL, &entry->data.data,
                    entry->data.length)) {
      return;
    }

    head = entry->next;
    if (head == nullptr) {
      tail = &head;
    }
    free(entry);
  }
}

void SplitSerialBuffer::SplitSerialBufferData::OnDataReceived(const void *data,
                                                              size_t length) {
  if (tud_cdc_connected()) {
    tud_cdc_write(data, length);
    tud_cdc_write_flush();
  }
}

//---------------------------------------------------------------------------

#endif // JAVELIN_SPLIT

//---------------------------------------------------------------------------
