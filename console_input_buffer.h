//---------------------------------------------------------------------------

#pragma once
#include <stdint.h>
#include <stdlib.h>

//---------------------------------------------------------------------------

class ConsoleInputBuffer {
public:
  static void Add(const uint8_t *data, size_t length) {
    instance.Add(data, length);
  };

  static void Process() { instance.Process(); }

private:
  struct Entry {
    Entry *next;
    size_t length;
    char data[0];

    static Entry *Create(const void *data, size_t length);
  };

  struct ConsoleInputBufferData {
    ConsoleInputBufferData() : head(nullptr), tail(&head) {}

    Entry *head;
    Entry **tail;

    void Add(const uint8_t *data, size_t length);
    void Process();
  };

  static ConsoleInputBufferData instance;
};

//---------------------------------------------------------------------------
