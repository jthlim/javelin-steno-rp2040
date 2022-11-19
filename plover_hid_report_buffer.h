//---------------------------------------------------------------------------

#pragma once
#include "hid_report_buffer.h"

//---------------------------------------------------------------------------

struct PloverHidReportBuffer : public HidReportBuffer<8> {
public:
  static PloverHidReportBuffer instance;
};

//---------------------------------------------------------------------------
