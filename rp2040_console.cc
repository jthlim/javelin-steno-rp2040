//---------------------------------------------------------------------------

#include "console_report_buffer.h"
#include "javelin/console.h"
#include JAVELIN_BOARD_CONFIG

//---------------------------------------------------------------------------

void ConsoleWriter::Write(const char *data, size_t length) {
  ConsoleReportBuffer::instance.SendData((const uint8_t *)data, length);
}

void Console::Flush() { ConsoleReportBuffer::instance.Flush(); }

//---------------------------------------------------------------------------
