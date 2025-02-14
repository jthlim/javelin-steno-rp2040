//---------------------------------------------------------------------------

#include "javelin/orthography.h"
#include "pico_spinlock.h"

//---------------------------------------------------------------------------

#if USE_ORTHOGRAPHY_CACHE

void StenoCompiledOrthography::LockCache() { spinlock17->Lock(); }
void StenoCompiledOrthography::UnlockCache() { spinlock17->Unlock(); }

#endif

//---------------------------------------------------------------------------
