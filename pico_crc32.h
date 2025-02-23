//---------------------------------------------------------------------------

#pragma once
#include <stdint.h>
#include <stdlib.h>

//---------------------------------------------------------------------------

struct PicoCrc32 {
public:
  static void Initialize();

  static uint32_t Hash(const void *data, size_t length);
};

//---------------------------------------------------------------------------
