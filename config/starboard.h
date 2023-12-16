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

#define JAVELIN_RGB 1
#define JAVELIN_RGB_COUNT 1
#define JAVELIN_RGB_PIN 23

#define JAVELIN_BUTTON_MATRIX 0
#define JAVELIN_BUTTON_PINS 1

// clang-format off
constexpr uint8_t BUTTON_PINS[] = {
    8,  7,  2,  1,  0,  5, /**/  22, 28, 27, 26, 18, 17,  // Top row
    9,  10, 3,  4,  6,     /**/      21, 20, 19, 15, 16,  // Middle row
               11, 12,     /**/      13, 14,              // Thumb row

    24, // usr button on MCU
};
constexpr uint32_t BUTTON_PIN_MASK = 0x1d7fffff;

#define BOOTSEL_BUTTON_INDEX 27

//
// Button indexes
//  0   1   2   3   4   5    |     6   7   8   9  10  11
// 12  13  14  15  16   5    |     6  17  18  19  20  21
//                 22  23    |    24  25
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
const size_t BUTTON_COUNT = 28;

const char *const MANUFACTURER_NAME = "Andrew Hess";
const char *const PRODUCT_NAME = "Starboard (Javelin)";
const int VENDOR_ID = 0xFEED;
const int PRODUCT_ID = 0xABCD;

//---------------------------------------------------------------------------
