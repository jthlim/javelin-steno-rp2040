//---------------------------------------------------------------------------

#include "console_input_buffer.h"
#include "console_buffer.h"
#include "javelin/console.h"
#include <string.h>

//---------------------------------------------------------------------------

ConsoleInputBuffer::ConsoleInputBufferData ConsoleInputBuffer::instance;

//---------------------------------------------------------------------------

QueueEntry<ConsoleInputBuffer::EntryData> *
ConsoleInputBuffer::ConsoleInputBufferData::CreateEntry(const void *data,
                                                        size_t length) {
  QueueEntry<EntryData> *entry =
      (QueueEntry<EntryData> *)malloc(sizeof(QueueEntry<EntryData>) + length);
  entry->data.length = length;
  entry->next = nullptr;
  memcpy(entry->data.data, data, length);
  return entry;
}

//---------------------------------------------------------------------------

void ConsoleInputBuffer::ConsoleInputBufferData::Add(const uint8_t *data,
                                                     size_t length) {
  QueueEntry<EntryData> *entry = CreateEntry(data, length);
  AddEntry(entry);
}

void ConsoleInputBuffer::ConsoleInputBufferData::Process() {
#if JAVELIN_SPLIT
  if (SplitTxRx::IsSlave() && isConnected) {
    // This will be dealt with using UpdateBuffer() instead.
    return;
  }
#endif

  while (head) {
    QueueEntry<EntryData> *entry = head;
    head = entry->next;
    if (head == nullptr) {
      tail = &head;
    }

    Console::instance.HandleInput(entry->data.data, entry->data.length);
    free(entry);
  }
  ConsoleBuffer::instance.Flush();
}

#if JAVELIN_SPLIT
void ConsoleInputBuffer::ConsoleInputBufferData::UpdateBuffer(
    TxBuffer &buffer) {
  while (head) {
    QueueEntry<EntryData> *entry = head;

    if (!buffer.Add(SplitHandlerId::CONSOLE, entry->data.data,
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

void ConsoleInputBuffer::ConsoleInputBufferData::OnDataReceived(
    const void *data, size_t length) {
  Add((const uint8_t *)data, length);
}
#endif

//---------------------------------------------------------------------------
