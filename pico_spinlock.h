//---------------------------------------------------------------------------

#pragma once
#include <stdint.h>

//---------------------------------------------------------------------------

struct PicoSpinLock {
public:
  void Lock() {
    while (spinlock == 0) {
      // Busy wait.
    }
  }

  void Unlock() { spinlock = (intptr_t)(this); }

private:
  volatile uint32_t spinlock;
};

//---------------------------------------------------------------------------

static PicoSpinLock *const spinlock0 = (PicoSpinLock *)0xd0000100;
static PicoSpinLock *const spinlock1 = (PicoSpinLock *)0xd0000104;
static PicoSpinLock *const spinlock2 = (PicoSpinLock *)0xd0000108;
static PicoSpinLock *const spinlock3 = (PicoSpinLock *)0xd000010c;
static PicoSpinLock *const spinlock4 = (PicoSpinLock *)0xd0000110;
static PicoSpinLock *const spinlock5 = (PicoSpinLock *)0xd0000114;
static PicoSpinLock *const spinlock6 = (PicoSpinLock *)0xd0000118;
static PicoSpinLock *const spinlock7 = (PicoSpinLock *)0xd000011c;
static PicoSpinLock *const spinlock8 = (PicoSpinLock *)0xd0000120;
static PicoSpinLock *const spinlock9 = (PicoSpinLock *)0xd0000124;
static PicoSpinLock *const spinlock10 = (PicoSpinLock *)0xd0000128;
static PicoSpinLock *const spinlock11 = (PicoSpinLock *)0xd000012c;
static PicoSpinLock *const spinlock12 = (PicoSpinLock *)0xd0000130;
static PicoSpinLock *const spinlock13 = (PicoSpinLock *)0xd0000134;
static PicoSpinLock *const spinlock14 = (PicoSpinLock *)0xd0000138;
static PicoSpinLock *const spinlock15 = (PicoSpinLock *)0xd000013c;
static PicoSpinLock *const spinlock16 = (PicoSpinLock *)0xd0000140;
static PicoSpinLock *const spinlock17 = (PicoSpinLock *)0xd0000144;
static PicoSpinLock *const spinlock18 = (PicoSpinLock *)0xd0000148;
static PicoSpinLock *const spinlock19 = (PicoSpinLock *)0xd000014c;
static PicoSpinLock *const spinlock20 = (PicoSpinLock *)0xd0000150;
static PicoSpinLock *const spinlock21 = (PicoSpinLock *)0xd0000154;
static PicoSpinLock *const spinlock22 = (PicoSpinLock *)0xd0000158;
static PicoSpinLock *const spinlock23 = (PicoSpinLock *)0xd000015c;
static PicoSpinLock *const spinlock24 = (PicoSpinLock *)0xd0000160;
static PicoSpinLock *const spinlock25 = (PicoSpinLock *)0xd0000164;
static PicoSpinLock *const spinlock26 = (PicoSpinLock *)0xd0000168;
static PicoSpinLock *const spinlock27 = (PicoSpinLock *)0xd000016c;
static PicoSpinLock *const spinlock28 = (PicoSpinLock *)0xd0000170;
static PicoSpinLock *const spinlock29 = (PicoSpinLock *)0xd0000174;
static PicoSpinLock *const spinlock30 = (PicoSpinLock *)0xd0000178;
static PicoSpinLock *const spinlock31 = (PicoSpinLock *)0xd000017c;

//---------------------------------------------------------------------------
