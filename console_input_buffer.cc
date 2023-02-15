//---------------------------------------------------------------------------

#include "console_input_buffer.h"
#include "console_buffer.h"
#include "javelin/console.h"
#include <string.h>

//---------------------------------------------------------------------------

ConsoleInputBuffer::ConsoleInputBufferData ConsoleInputBuffer::instance;

//---------------------------------------------------------------------------

ConsoleInputBuffer::Entry *ConsoleInputBuffer::Entry::Create(const void *data,
                                                             size_t length) {
  Entry *entry = (Entry *)malloc(sizeof(Entry) + length);
  entry->length = length;
  entry->next = nullptr;
  memcpy(entry->data, data, length);
  return entry;
}

//---------------------------------------------------------------------------

void ConsoleInputBuffer::ConsoleInputBufferData::Add(const uint8_t *data,
                                                     size_t length) {
  Entry *entry = Entry::Create(data, length);
  *tail = entry;
  tail = &entry->next;
}

void ConsoleInputBuffer::ConsoleInputBufferData::Process() {
  while (head) {
    Entry *entry = head;
    head = entry->next;
    if (head == nullptr) {
      tail = &head;
    }

    Console::instance.HandleInput(entry->data, entry->length);
    free(entry);
  }
  ConsoleBuffer::instance.Flush();
}

//---------------------------------------------------------------------------
