//---------------------------------------------------------------------------

#pragma once
#include <stddef.h>
#include <stdint.h>

//---------------------------------------------------------------------------

struct StenoConfigBlock;
struct StenoOrthography;
struct StenoDictionaryCollection;

//---------------------------------------------------------------------------

#define JAVELIN_USE_EMBEDDED_STENO 1
#define JAVELIN_USE_USER_DICTIONARY 1
#define JAVELIN_USB_MILLIAMPS 100
#define BOOTSEL_BUTTON_INDEX 26

#define JAVELIN_BUTTON_MATRIX 0
#define JAVELIN_BUTTON_TOUCH 1

// clang-format off
constexpr uint8_t BUTTON_TOUCH_PINS[26] = {
           0,         /**/           25,
   8,  6,  2,  3,  1, /**/ 24, 22, 23, 19, 17, 12,
   9,  7,  5,  4,     /**/     21, 20, 18, 16, 13,
              10, 11, /**/ 15, 14,
};
constexpr uint32_t BUTTON_TOUCH_PIN_MASK = 0x03ffffff;
constexpr float BUTTON_TOUCH_THRESHOLD = 1.25f;
// clang-format on

const StenoConfigBlock *const STENO_CONFIG_BLOCK_ADDRESS =
    (const StenoConfigBlock *)0x10040000;
const uint8_t *const SCRIPT_BYTE_CODE = (const uint8_t *)0x10040100;
const StenoOrthography *const ORTHOGRAPHY_ADDRESS =
    (const StenoOrthography *)0x10042000;
const uint8_t *const STENO_WORD_LIST_ADDRESS = (const uint8_t *)0x10044000;
const StenoDictionaryCollection *const STENO_MAP_DICTIONARY_COLLECTION_ADDRESS =
    (const StenoDictionaryCollection *)0x10400000;
const uint8_t *const STENO_USER_DICTIONARY_ADDRESS =
    (const uint8_t *)0x10fc0000;
const size_t STENO_USER_DICTIONARY_SIZE = 0x40000;

const size_t MAXIMUM_MAP_DICTIONARY_SIZE = 0xbc0000;
const size_t MAXIMUM_BUTTON_SCRIPT_SIZE = 0x1f00;
const size_t BUTTON_COUNT = 27;

const char *const MANUFACTURER_NAME = "stenokeyboards";
const char *const PRODUCT_NAME = "Asterisk (Javelin)";
const int VENDOR_ID = 0x9000;

//---------------------------------------------------------------------------
