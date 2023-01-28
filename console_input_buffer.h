//---------------------------------------------------------------------------

#pragma once
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

//---------------------------------------------------------------------------

class ConsoleInputBuffer {
public:
  void Add(const uint8_t *data, size_t length);
  void Process();

  static ConsoleInputBuffer instance;

private:
  ConsoleInputBuffer() : head(nullptr), tail(&head) {}

  struct Entry {
    Entry *next;
    size_t length;
    char data[0];

    static Entry *Create(const void *data, size_t length);
  };

  Entry *head;
  Entry **tail;
};

//---------------------------------------------------------------------------
