//---------------------------------------------------------------------------

#include "console_report_buffer.h"
#include "usb_descriptors.h"

#include <string.h>

//---------------------------------------------------------------------------

ConsoleReportBuffer ConsoleReportBuffer::instance;

//---------------------------------------------------------------------------

ConsoleReportBuffer::ConsoleReportBuffer() : reportBuffer(ITF_NUM_CONSOLE) {}

void ConsoleReportBuffer::SendData(const uint8_t *data, size_t length) {
  // Fill up the previous buffer if it's not empty.
  if (bufferSize != 0) {
    int remaining = MAX_BUFFER_SIZE - bufferSize;
    int bytesToAdd = length > remaining ? remaining : length;
    memcpy(buffer + bufferSize, data, bytesToAdd);
    bufferSize += bytesToAdd;

    data += bytesToAdd;
    length -= bytesToAdd;

    if (bufferSize == MAX_BUFFER_SIZE) {
      reportBuffer.SendReport(0, buffer, MAX_BUFFER_SIZE);
      bufferSize = 0;
    }

    if (length == 0) {
      return;
    }
  }

  // Send all full length requests.
  while (length >= MAX_BUFFER_SIZE) {
    reportBuffer.SendReport(0, data, MAX_BUFFER_SIZE);
    data += MAX_BUFFER_SIZE;
    length -= MAX_BUFFER_SIZE;
  }

  if (length > 0) {
    memcpy(buffer, data, length);
    bufferSize = length;
  }
}

//---------------------------------------------------------------------------

void ConsoleReportBuffer::Flush() {
  if (bufferSize > 0) {
    int remaining = MAX_BUFFER_SIZE - bufferSize;
    memset(buffer + bufferSize, 0, remaining);
    reportBuffer.SendReport(0, buffer, MAX_BUFFER_SIZE);
    bufferSize = 0;
  }
}

//---------------------------------------------------------------------------
