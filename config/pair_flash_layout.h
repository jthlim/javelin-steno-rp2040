//---------------------------------------------------------------------------

#pragma once
#include <stddef.h>
#include <stdint.h>

//---------------------------------------------------------------------------

struct ScriptStorageData;
struct StenoConfigBlock;

//---------------------------------------------------------------------------

#define JAVELIN_USE_SCRIPT_STORAGE 1

//---------------------------------------------------------------------------

const StenoConfigBlock *const STENO_CONFIG_BLOCK_ADDRESS =
    (const StenoConfigBlock *)0x10040000;
const uint8_t *const SCRIPT_BYTE_CODE = (const uint8_t *)0x10040100;
const size_t MAXIMUM_BUTTON_SCRIPT_SIZE = 0x1f00;

static const struct ScriptStorageData *const SCRIPT_STORAGE_ADDRESS =
    (struct ScriptStorageData *)0x10050000;
static const size_t MAXIMUM_SCRIPT_STORAGE_SIZE = 0x20000;

//---------------------------------------------------------------------------
