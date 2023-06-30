//---------------------------------------------------------------------------

#include "javelin/orthography.h"
#include "rp2040_spinlock.h"

//---------------------------------------------------------------------------

#if USE_ORTHOGRAPHY_CACHE

void StenoCompiledOrthography::LockCache() { spinlock17->Lock(); }
void StenoCompiledOrthography::UnlockCache() { spinlock17->Unlock(); }

#endif

//---------------------------------------------------------------------------
