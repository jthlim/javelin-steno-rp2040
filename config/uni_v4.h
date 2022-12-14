//---------------------------------------------------------------------------

#pragma once
#include <stdint.h>
#include <stdlib.h>

//---------------------------------------------------------------------------

struct StenoConfigBlock;
struct StenoOrthography;
struct StenoMapDictionaryCollection;

//---------------------------------------------------------------------------

#define USE_USER_DICTIONARY 1

constexpr uint8_t COLUMN_PINS[] = {24, 23, 21, 20, 19, 6, 5, 4, 3, 2, 1};
constexpr uint32_t COLUMN_PIN_MASK = 0x1b8007e;
constexpr uint8_t ROW_PINS[] = {25, 18, 17};
constexpr uint32_t ROW_PIN_MASK = 0x2060000;

// clang-format off
constexpr int8_t KEY_MAP[3][16] = {
  {  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10 },
  { 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21 },
  { -1, -1, 22, 23, 24, 25, 26, 27, -1, -1, -1 },
};
// clang-format on

const StenoOrthography *const ORTHOGRAPHY_ADDRESS =
    (const StenoOrthography *)0x1003e000;
const uint8_t *const STENO_WORD_LIST_ADDRESS = (const uint8_t *)0x10040000;
const StenoConfigBlock *const STENO_CONFIG_BLOCK_ADDRESS =
    (const StenoConfigBlock *)0x103ff000;
const uint8_t *const BUTTON_MANAGER_BYTE_CODE = (const uint8_t *)0x103ff100;
const StenoMapDictionaryCollection
    *const STENO_MAP_DICTIONARY_COLLECTION_ADDRESS =
        (const StenoMapDictionaryCollection *)0x10400000;
const uint8_t *const STENO_USER_DICTIONARY_ADDRESS =
    (const uint8_t *)0x10f70000;
const size_t STENO_USER_DICTIONARY_SIZE = 0x80000;

const char *const MANUFACTURER_NAME = "stenokeyboards";
const char *const PRODUCT_NAME = "The Uni (Javelin)";
const int VENDOR_ID = 0x9000;

//---------------------------------------------------------------------------
