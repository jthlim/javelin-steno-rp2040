//---------------------------------------------------------------------------

#pragma once
#include <stdint.h>
#include <stdlib.h>

//---------------------------------------------------------------------------

struct StenoConfigBlock;
struct StenoOrthography;
struct StenoMapDictionaryCollection;

//---------------------------------------------------------------------------

#define JAVELIN_USE_EMBEDDED_STENO 1
#define JAVELIN_USE_USER_DICTIONARY 1
#define JAVELIN_USB_MILLIAMPS 500

#define JAVELIN_RGB_COUNT 54
#define JAVELIN_RGB_MASTER_COUNT 27
#define JAVELIN_RGB_PIN 0
#define JAVELIN_USE_RGB_MAP 1

#define JAVELIN_SPLIT 1
#define JAVELIN_SPLIT_TX_PIN 1
#define JAVELIN_SPLIT_RX_PIN 1
#define JAVELIN_SPLIT_IS_MASTER 1
#define JAVELIN_SPLIT_IS_LEFT 1
// #define JAVELIN_SPLIT_SIDE_PIN xx

// #define JAVELIN_OLED_DRIVER 1306
// #define JAVELIN_OLED_WIDTH 128
// #define JAVELIN_OLED_HEIGHT 64
// #define JAVELIN_OLED_I2C i2c1
// #define JAVELIN_OLED_SDA_PIN 2
// #define JAVELIN_OLED_SCL_PIN 3
// #define JAVELIN_OLED_I2C_ADDRESS 0x3c
// #define JAVELIN_OLED_ROTATION 180

constexpr uint8_t LEFT_COLUMN_PINS[] = {29, 28, 27, 26, 22, 20};
constexpr uint32_t LEFT_COLUMN_PIN_MASK = 0x3c500000;
constexpr uint8_t LEFT_ROW_PINS[] = {4, 5, 6, 7};
constexpr uint32_t LEFT_ROW_PIN_MASK = 0x000000f0;

constexpr uint8_t RIGHT_COLUMN_PINS[] = {20, 22, 26, 27, 28, 29};
constexpr uint32_t RIGHT_COLUMN_PIN_MASK = 0x3c500000;
constexpr uint8_t RIGHT_ROW_PINS[] = {4, 5, 6, 7};
constexpr uint32_t RIGHT_ROW_PIN_MASK = 0x000000f0;

// clang-format off
//
// Button indexes
//  0   1   2   3   4   5    |     6   7   8   9  10  11
// 12  13  14  15  16  17    |    18  19  20  21  22  23
// 24  25  26  27  28  29    |    30  31  32  33  34  35
//               36  37  38  |  39  40  41
//
// RGB indices
//  Back:
//      2         1         0    |    27        28        29
//                               |
//      3         4         5    |    32        31        30
//                               |
//
//  Front:
//     24  23  18  17  10  09    |    36  37  44  45  50  51
//     25  22  19  16  11  08    |    35  38  43  46  49  52
//     26  21  20  15  12  07    |    34  39  42  47  48  53
//                   14  13  06  |  33  40  41

constexpr int8_t LEFT_KEY_MAP[4][6] = {
  {   0,  1,  2,  3,  4,  5, },
  {  12, 13, 14, 15, 16, 17, },
  {  24, 25, 26, 27, 28, 29, },
  {  -1, -1, -1, 36, 37, 38, },
};

constexpr int8_t RIGHT_KEY_MAP[4][6] = {
  {   6,  7,  8,  9, 10, 11, },
  {  18, 19, 20, 21, 22, 23, },
  {  30, 31, 32, 34, 35, 36, },
  {  39, 40, 41, -1, -1, -1, },
};

constexpr uint8_t RGB_MAP[54] = {
  24, 23, 18, 17, 10,  9,   36, 37, 44, 45, 50, 51,
  25, 22, 19, 16, 11,  8,   35, 38, 43, 46, 49, 52,
  26, 21, 20, 15, 12,  7,   34, 39, 42, 47, 48, 53,
              14, 13,  6,   33, 40, 41,

  // Underglow
  2, 1, 0, 27, 28, 29,
  3, 4, 5, 32, 31, 30,
};

// clang-format on

const StenoConfigBlock *const STENO_CONFIG_BLOCK_ADDRESS =
    (const StenoConfigBlock *)0x10040000;
const uint8_t *const BUTTON_MANAGER_BYTE_CODE = (const uint8_t *)0x10040100;
const StenoOrthography *const ORTHOGRAPHY_ADDRESS =
    (const StenoOrthography *)0x10042000;
const uint8_t *const STENO_WORD_LIST_ADDRESS = (const uint8_t *)0x10044000;
const StenoMapDictionaryCollection
    *const STENO_MAP_DICTIONARY_COLLECTION_ADDRESS =
        (const StenoMapDictionaryCollection *)0x10400000;
const uint8_t *const STENO_USER_DICTIONARY_ADDRESS =
    (const uint8_t *)0x10fc0000;
const size_t STENO_USER_DICTIONARY_SIZE = 0x40000;

const char *const MANUFACTURER_NAME = "foostan";
const char *const PRODUCT_NAME = "Corne (Javelin)";
const int VENDOR_ID = 0x4653;
// const int PRODUCT_ID = 0x0001;

//---------------------------------------------------------------------------