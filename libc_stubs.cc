//---------------------------------------------------------------------------
//
// Definitions to silence linker warnings
//
//---------------------------------------------------------------------------

extern "C" {

//---------------------------------------------------------------------------

void _close(void) {}
void _fstat(void) {}
void _isatty(void) {}
void _lseek(void) {}

//---------------------------------------------------------------------------
}
//---------------------------------------------------------------------------
