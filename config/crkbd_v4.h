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
#define JAVELIN_USE_SCRIPT_STORAGE 1
#define JAVELIN_USB_MILLIAMPS 500

#define JAVELIN_RGB 1
#define JAVELIN_RGB_COUNT 46
#define JAVELIN_RGB_LEFT_COUNT 23
#define JAVELIN_RGB_PIN 10
#define JAVELIN_USE_RGB_MAP 1

#define JAVELIN_SPLIT 1
#define JAVELIN_SPLIT_TX_PIN 12
#define JAVELIN_SPLIT_RX_PIN 12
#define JAVELIN_SPLIT_IS_MASTER 1
#define JAVELIN_SPLIT_IS_LEFT 1
// #define JAVELIN_SPLIT_SIDE_PIN xx
#define JAVELIN_SPLIT_TX_RX_BUFFER_SIZE 512

#define JAVELIN_BUTTON_MATRIX 0
#define JAVELIN_BUTTON_PINS 1

// clang-format off
constexpr uint8_t BUTTON_PINS[] = {
   22, 20, 23, 26, 29,  0,  4, /**/  0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f,
   19, 18, 24, 27,  1,  2,  8, /**/  0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f,
   17, 16, 25, 28,  3,  9,     /**/        0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f,
                 14, 15, 11,   /**/  0x7f, 0x7f, 0x7f,
};
constexpr uint32_t BUTTON_PIN_MASK = 0x3fbfcb1f;

// clang-format off
//
// Button indexes
//  0   1   2   3   4   5   6 |   7   8   9  10  11  12  13
// 14  15  16  17  18  19  20 |  21  22  23  24  25  26  27
// 28  29  30  31  32  33     |      34  35  36  37  38  39
//               40  41   42  |   43   44  45
//
//  Front:
// 18  17  12  11  04  03  21 |  44  26  27  34  35  40  41
// 19  16  13  10  05  02  22 |  45  25  28  33  36  39  42
// 20  15  14  09  06  01     |      24  29  32  37  38  43
//               08  07  00   |    23  30  31

constexpr uint8_t RGB_MAP[JAVELIN_RGB_COUNT] = {
  18, 17, 12, 11, 04, 03, 21, /**/ 44, 26, 27, 34, 35, 40, 41,
  19, 16, 13, 10, 05, 02, 22, /**/ 45, 25, 28, 33, 36, 39, 42,
  20, 15, 14,  9, 06, 01,     /**/     24, 29, 32, 37, 38, 43,
                  8,  7,  0,  /**/  23, 30, 31
};

// clang-format on

#define JAVELIN_SCRIPT_CONFIGURATION                                           \
  R"({"name":"CorneV4","layout":[{"x":0,"y":0.3},{"x":1,"y":0.3},{"x":2,"y":0.1},{"x":3,"y":0},{"x":4,"y":0.1},{"x":5,"y":0.2},{"x":6,"y":0.7},{"x":8,"y":0.7},{"x":9,"y":0.2},{"x":10,"y":0.1},{"x":11,"y":0},{"x":12,"y":0.1},{"x":13,"y":0.3},{"x":14,"y":0.3},{"x":0,"y":1.3},{"x":1,"y":1.3},{"x":2,"y":1.1},{"x":3,"y":1},{"x":4,"y":1.1},{"x":5,"y":1.2},{"x":6,"y":1.7},{"x":8,"y":1.7},{"x":9,"y":1.2},{"x":10,"y":1.1},{"x":11,"y":1},{"x":12,"y":1.1},{"x":13,"y":1.3},{"x":14,"y":1.3},{"x":0,"y":2.3},{"x":1,"y":2.3},{"x":2,"y":2.1},{"x":3,"y":2},{"x":4,"y":2.1},{"x":5,"y":2.2},{"x":9,"y":2.2},{"x":10,"y":2.1},{"x":11,"y":2},{"x":12,"y":2.1},{"x":13,"y":2.3},{"x":14,"y":2.3},{"x":3.5,"y":3.4},{"x":4.7,"y":3.58,"r":0.26},{"x":5.95,"y":3.6,"w":1,"h":1.5,"r":0.52},{"x":8.05,"y":3.6,"w":1,"h":1.5,"r":-0.52},{"x":9.3,"y":3.58,"r":-0.26},{"x":10.5,"y":3.4}]})"

const StenoConfigBlock *const STENO_CONFIG_BLOCK_ADDRESS =
    (const StenoConfigBlock *)0x10040000;
const uint8_t *const SCRIPT_BYTE_CODE = (const uint8_t *)0x10040100;
const StenoOrthography *const ORTHOGRAPHY_ADDRESS =
    (const StenoOrthography *)0x10042000;
const uint8_t *const STENO_WORD_LIST_ADDRESS = (const uint8_t *)0x10044000;
static const struct ScriptStorageData *const SCRIPT_STORAGE_ADDRESS =
    (struct ScriptStorageData *)0x103e0000;
static const size_t MAXIMUM_SCRIPT_STORAGE_SIZE = 0x20000;
const StenoDictionaryCollection *const STENO_MAP_DICTIONARY_COLLECTION_ADDRESS =
    (const StenoDictionaryCollection *)0x10400000;
const uint8_t *const STENO_USER_DICTIONARY_ADDRESS =
    (const uint8_t *)0x10fc0000;
const size_t STENO_USER_DICTIONARY_SIZE = 0x40000;

const size_t MAXIMUM_MAP_DICTIONARY_SIZE = 0xbc0000;
const size_t MAXIMUM_BUTTON_SCRIPT_SIZE = 0x1f00;
const size_t BUTTON_COUNT = 46;

const char *const MANUFACTURER_NAME = "foostan";
const char *const PRODUCT_NAME = "Corne (Javelin)";
const int VENDOR_ID = 0x4653;
// const int PRODUCT_ID = 0x0001;

//---------------------------------------------------------------------------