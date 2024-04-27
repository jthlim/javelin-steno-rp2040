//---------------------------------------------------------------------------

#include JAVELIN_BOARD_CONFIG

#include "javelin/flash.h"
#include <hardware/flash.h>
#include <hardware/sync.h>
#include <string.h>

//---------------------------------------------------------------------------

static bool IsWritableRange(const void *p) {
  return p >= STENO_CONFIG_BLOCK_ADDRESS;
}

void Flash::Erase(const void *target, size_t size) {
  if (!IsWritableRange(target)) {
    return;
  }

  const uint8_t *const t = (const uint8_t *)target;

  size_t eraseStart = 0;
  size_t eraseEnd = 0;

  for (size_t i = 0; i < size; i += 4096) {
    if (RequiresErase(t + i, 4096)) {
      if (eraseStart != eraseEnd) {
        eraseEnd = i + 4096;
      } else {
        eraseStart = i;
        eraseEnd = i + 4096;
      }
    }
  }

  const size_t eraseSize = eraseEnd - eraseStart;
  if (eraseSize != 0) {
    instance.erasedBytes += eraseSize;

    const uint32_t interrupts = save_and_disable_interrupts();
    flash_range_erase((intptr_t)t + eraseStart - XIP_BASE, eraseSize);
    restore_interrupts(interrupts);
  }
}

void Flash::Write(const void *target, const void *data, size_t size) {
  if (!IsWritableRange(target)) {
    return;
  }

  const uint8_t *const t = (const uint8_t *)target;
  const uint8_t *const d = (const uint8_t *)data;

  size_t eraseStart = 0;
  size_t eraseEnd = 0;

  for (size_t i = 0; i < size; i += 4096) {
    if (RequiresErase(t + i, d + i, 4096)) {
      if (eraseStart != eraseEnd) {
        eraseEnd = i + 4096;
      } else {
        eraseStart = i;
        eraseEnd = i + 4096;
      }
    }
  }

  const size_t eraseSize = eraseEnd - eraseStart;
  if (eraseSize != 0) {
    instance.erasedBytes += eraseSize;

    const uint32_t interrupts = save_and_disable_interrupts();
    flash_range_erase((intptr_t)t + eraseStart - XIP_BASE, eraseSize);
    restore_interrupts(interrupts);
  }

  size_t programStart = 0;
  size_t programEnd = 0;
  for (size_t i = 0; i < size; i += 256) {
    if (RequiresProgram(t + i, d + i, 256)) {
      if (programStart != programEnd) {
        programEnd = i + 256;
      } else {
        programStart = i;
        programEnd = i + 256;
      }
    }
  }

  const size_t programSize = programEnd - programStart;
  if (programSize) {
    instance.programmedBytes += programSize;

    const uint32_t interrupts = save_and_disable_interrupts();
    flash_range_program((intptr_t)t + programStart - XIP_BASE, d + programStart,
                        programSize);
    restore_interrupts(interrupts);
  }

  // Do a check, to ensure it is programmed accurately.
  if (memcmp(target, data, size) != 0) {
    // If it didn't, then do a full erase/program cycle.
    instance.erasedBytes += size;
    instance.programmedBytes += size;
    instance.reprogrammedBytes += size;

    const uint32_t interrupts = save_and_disable_interrupts();
    flash_range_erase((intptr_t)t - XIP_BASE, size);
    flash_range_program((intptr_t)t - XIP_BASE, d, size);
    restore_interrupts(interrupts);
  }
}

//---------------------------------------------------------------------------
