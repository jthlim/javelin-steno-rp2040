//---------------------------------------------------------------------------

#pragma once
#include "rp2040_dma.h"
#include <stdlib.h>

//---------------------------------------------------------------------------

struct Rp2040SpinLock {
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

static Rp2040SpinLock *const spinlock0 = (Rp2040SpinLock *)0xd0000100;
static Rp2040SpinLock *const spinlock1 = (Rp2040SpinLock *)0xd0000104;
static Rp2040SpinLock *const spinlock2 = (Rp2040SpinLock *)0xd0000108;
static Rp2040SpinLock *const spinlock3 = (Rp2040SpinLock *)0xd000010c;
static Rp2040SpinLock *const spinlock4 = (Rp2040SpinLock *)0xd0000110;
static Rp2040SpinLock *const spinlock5 = (Rp2040SpinLock *)0xd0000114;
static Rp2040SpinLock *const spinlock6 = (Rp2040SpinLock *)0xd0000118;
static Rp2040SpinLock *const spinlock7 = (Rp2040SpinLock *)0xd000011c;
static Rp2040SpinLock *const spinlock8 = (Rp2040SpinLock *)0xd0000120;
static Rp2040SpinLock *const spinlock9 = (Rp2040SpinLock *)0xd0000124;
static Rp2040SpinLock *const spinlock10 = (Rp2040SpinLock *)0xd0000128;
static Rp2040SpinLock *const spinlock11 = (Rp2040SpinLock *)0xd000012c;
static Rp2040SpinLock *const spinlock12 = (Rp2040SpinLock *)0xd0000130;
static Rp2040SpinLock *const spinlock13 = (Rp2040SpinLock *)0xd0000134;
static Rp2040SpinLock *const spinlock14 = (Rp2040SpinLock *)0xd0000138;
static Rp2040SpinLock *const spinlock15 = (Rp2040SpinLock *)0xd000013c;
static Rp2040SpinLock *const spinlock16 = (Rp2040SpinLock *)0xd0000140;
static Rp2040SpinLock *const spinlock17 = (Rp2040SpinLock *)0xd0000144;
static Rp2040SpinLock *const spinlock18 = (Rp2040SpinLock *)0xd0000148;
static Rp2040SpinLock *const spinlock19 = (Rp2040SpinLock *)0xd000014c;
static Rp2040SpinLock *const spinlock20 = (Rp2040SpinLock *)0xd0000150;
static Rp2040SpinLock *const spinlock21 = (Rp2040SpinLock *)0xd0000154;
static Rp2040SpinLock *const spinlock22 = (Rp2040SpinLock *)0xd0000158;
static Rp2040SpinLock *const spinlock23 = (Rp2040SpinLock *)0xd000015c;
static Rp2040SpinLock *const spinlock24 = (Rp2040SpinLock *)0xd0000160;
static Rp2040SpinLock *const spinlock25 = (Rp2040SpinLock *)0xd0000164;
static Rp2040SpinLock *const spinlock26 = (Rp2040SpinLock *)0xd0000168;
static Rp2040SpinLock *const spinlock27 = (Rp2040SpinLock *)0xd000016c;
static Rp2040SpinLock *const spinlock28 = (Rp2040SpinLock *)0xd0000170;
static Rp2040SpinLock *const spinlock29 = (Rp2040SpinLock *)0xd0000174;
static Rp2040SpinLock *const spinlock30 = (Rp2040SpinLock *)0xd0000178;
static Rp2040SpinLock *const spinlock31 = (Rp2040SpinLock *)0xd000017c;

//---------------------------------------------------------------------------
