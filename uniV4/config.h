//---------------------------------------------------------------------------

#pragma once
#include "javelin/orthography.h"
#include "javelin/steno_key_state.h"

//---------------------------------------------------------------------------

constexpr uint8_t COLUMN_PINS[] = {24, 23, 21, 20, 19, 6, 5, 4, 3, 2, 1};
constexpr uint32_t COLUMN_PIN_MASK = 0x1b8007e;
constexpr uint8_t ROW_PINS[] = {25, 18, 17};
constexpr uint32_t ROW_PIN_MASK = 0x2060000;

// clang-format off
#define SK StenoKey
constexpr StenoKey KEY_MAP[3][16] = {
  { SK::S1,   SK::TL,   SK::PL,   SK::HL, SK::STAR1, SK::STAR3, SK::FR, SK::PR,   SK::LR,   SK::TR,   SK::DR   },
  { SK::S2,   SK::KL,   SK::WL,   SK::RL, SK::STAR2, SK::STAR4, SK::RR, SK::BR,   SK::GR,   SK::SR,   SK::ZR   },
  { SK::NONE, SK::NONE, SK::NUM1, SK::A,  SK::O,     SK::E,     SK::U,  SK::NUM2, SK::NONE, SK::NONE, SK::NONE },
};
#undef SK
// clang-format on

#define USE_USER_DICTIONARY 1

const StenoOrthography *const ORTHOGRAPHY_ADDRESS =
    (const StenoOrthography *)0x1003e000;
const uint8_t *const STENO_WORD_LIST_ADDRESS = (const uint8_t *)0x10040000;
const uint8_t *const STENO_USER_DICTIONARY_ADDRESS =
    (const uint8_t *)0x10f70000;
const size_t STENO_USER_DICTIONARY_SIZE = 0x80000;
const uint8_t *const STENO_CONFIG_BLOCK_ADDRESS = (const uint8_t *)0x103ff000;
const void *const STENO_MAIN_DICTIONARY_ADDRESS = (const void *)0x10400000;

const char *const MANUFACTURER_NAME = "stenokeyboards";
const char *const PRODUCT_NAME = "The Uni (Javelin)";
const int VENDOR_ID = 0x9000;

//---------------------------------------------------------------------------
