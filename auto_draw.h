//---------------------------------------------------------------------------

#pragma once
#include JAVELIN_BOARD_CONFIG
#include "javelin/processor/passthrough.h"
#include "javelin/static_allocate.h"

//---------------------------------------------------------------------------

#if JAVELIN_OLED_DRIVER

enum class AutoDraw : uint8_t {
  NONE,
  PAPER_TAPE,
  STENO_LAYOUT,
  WPM,
  STROKES,
};

class StenoStrokeCapture final : public StenoPassthrough {
public:
  StenoStrokeCapture(StenoProcessorElement *next) : StenoPassthrough(next) {}

  void Process(const StenoKeyState &value, StenoAction action) override;
  void Tick() override;

  void Update(bool onStrokeInput);

  static void SetAutoDraw_Binding(void *context, const char *commandLine);
  void SetAutoDraw(int displayId, AutoDraw autoDrawId);

  static JavelinStaticAllocate<StenoStrokeCapture> container;

private:
  static const size_t MAXIMUM_STROKE_COUNT = 32;
  size_t strokeCount = 0;
  uint32_t lastUpdateTime = 0;
#if JAVELIN_SPLIT
  AutoDraw autoDraw[2];
#else
  AutoDraw autoDraw[1];
#endif
  StenoStroke strokes[MAXIMUM_STROKE_COUNT];

  static void DrawWpm(int displayId);
  void DrawStrokes(int displayId);
};

#endif

//---------------------------------------------------------------------------
