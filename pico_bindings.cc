//---------------------------------------------------------------------------

#include "bootrom.h"
#include "console_buffer.h"
#include "font.h"
#include "hid_keyboard_report_builder.h"
#include "hid_report_buffer.h"
#include "javelin/button_manager.h"
#include "javelin/clock.h"
#include "javelin/config_block.h"
#include "javelin/console.h"
#include "javelin/dictionary/corrupted_dictionary.h"
#include "javelin/dictionary/debug_dictionary.h"
#include "javelin/dictionary/dictionary.h"
#include "javelin/dictionary/dictionary_list.h"
#include "javelin/dictionary/emily_symbols_dictionary.h"
#include "javelin/dictionary/jeff_numbers_dictionary.h"
#include "javelin/dictionary/jeff_phrasing_dictionary.h"
#include "javelin/dictionary/jeff_show_stroke_dictionary.h"
#include "javelin/dictionary/main_dictionary.h"
#include "javelin/dictionary/map_dictionary.h"
#include "javelin/dictionary/map_dictionary_definition.h"
#include "javelin/dictionary/reverse_auto_suffix_dictionary.h"
#include "javelin/dictionary/reverse_map_dictionary.h"
#include "javelin/dictionary/reverse_prefix_dictionary.h"
#include "javelin/dictionary/user_dictionary.h"
#include "javelin/display.h"
#include "javelin/engine.h"
#include "javelin/gpio.h"
#include "javelin/key.h"
#include "javelin/orthography.h"
#include "javelin/processor/all_up.h"
#include "javelin/processor/first_up.h"
#include "javelin/processor/gemini.h"
#include "javelin/processor/jeff_modifiers.h"
#include "javelin/processor/passthrough.h"
#include "javelin/processor/plover_hid.h"
#include "javelin/processor/processor_list.h"
#include "javelin/processor/repeat.h"
#include "javelin/rgb.h"
#include "javelin/serial_port.h"
#include "javelin/static_allocate.h"
#include "javelin/steno_key_code.h"
#include "javelin/steno_key_code_emitter.h"
#include "javelin/word_list.h"
#include "javelin/wpm_tracker.h"
#include "plover_hid_report_buffer.h"
#include "rp2040_divider.h"
#include "split_hid_report_buffer.h"
#include "split_serial_buffer.h"
#include "split_tx_rx.h"
#include "ssd1306.h"
#include "usb_descriptors.h"
#include "ws2812.h"

#include <hardware/clocks.h>
#include <hardware/flash.h>
#include <hardware/timer.h>
#include <hardware/watchdog.h>
#include <malloc.h>
#include <tusb.h>

#include JAVELIN_BOARD_CONFIG

//---------------------------------------------------------------------------

#define TRACE_RELEASE_PROCESSING_TIME 0
#define ENABLE_DEBUG_COMMAND 0

//---------------------------------------------------------------------------

#if JAVELIN_OLED_DRIVER

enum class AutoDraw : uint8_t {
  NONE,
  PAPER_TAPE,
  STENO_LAYOUT,
  WPM,
};

class StenoStrokeCapture : public StenoPassthrough {
public:
  StenoStrokeCapture(StenoProcessorElement *next) : StenoPassthrough(next) {}

  void Process(const StenoKeyState &value, StenoAction action) {
    if (action == StenoAction::TRIGGER) {
      if (strokeCount < MAXIMUM_STROKE_COUNT) {
        strokes[strokeCount++] = value.ToStroke();
      } else {
        memmove(&strokes[0], &strokes[1],
                sizeof(StenoStroke) * (MAXIMUM_STROKE_COUNT - 1));
        strokes[MAXIMUM_STROKE_COUNT - 1] = value.ToStroke();
      }
      Update(true);
    }
    StenoPassthrough::Process(value, action);
  }

  void Update(bool onStrokeInput) {
#if JAVELIN_SPLIT
    for (int displayId = 0; displayId < 2; ++displayId) {
#else
    const int displayId = 0;
    {
#endif
      switch (autoDraw[displayId]) {
      case AutoDraw::NONE:
        // Do nothing
        break;
      case AutoDraw::PAPER_TAPE:
        Ssd1306::DrawPaperTape(displayId, strokes, strokeCount);
        break;
      case AutoDraw::STENO_LAYOUT:
        Ssd1306::DrawStenoLayout(displayId, strokeCount != 0
                                                ? strokes[strokeCount - 1]
                                                : StenoStroke(0));
        break;
      case AutoDraw::WPM:
        if (!onStrokeInput) {
          DrawWpm(displayId);
        }
        break;
      }
    }
  }

  static void SetAutoDraw_Binding(void *context, const char *commandLine);
  void SetAutoDraw(int displayId, AutoDraw autoDrawId) {
#if JAVELIN_SPLIT
    if (displayId < 0 || displayId >= 2) {
      return;
    }
#else
    displayId = 0;
#endif
    autoDraw[displayId] = autoDrawId;
    Update(false);
  }

  virtual void Tick() {
    StenoPassthrough::Tick();

    uint32_t now = time_us_32();
    if (now - lastUpdateTime < 1000) {
      return;
    }
    lastUpdateTime = now;

#if JAVELIN_SPLIT
    for (int displayId = 0; displayId < 2; ++displayId) {
#else
    const int displayId = 0;
    {
#endif
      switch (autoDraw[displayId]) {
      case AutoDraw::NONE:
      case AutoDraw::PAPER_TAPE:
      case AutoDraw::STENO_LAYOUT:
        break;

      case AutoDraw::WPM:
        DrawWpm(displayId);
        break;
      }
    }
  }

private:
  static const size_t MAXIMUM_STROKE_COUNT = 16;
  uint8_t strokeCount = 0;
  uint32_t lastUpdateTime = 0;
#if JAVELIN_SPLIT
  AutoDraw autoDraw[2];
#else
  AutoDraw autoDraw[1];
#endif
  StenoStroke strokes[MAXIMUM_STROKE_COUNT];

  static void DrawWpm(int displayId) {
    Display::Clear(displayId);

    char buffer[16];
    itoa(WpmTracker::instance.Get5sWpm(), buffer, 10);
    const Font *font = &Font::LARGE_DIGITS;
    int textWidth = font->GetStringWidth(buffer);
    Ssd1306::DrawText(displayId, JAVELIN_OLED_WIDTH / 2 - textWidth / 2,
                      JAVELIN_OLED_HEIGHT / 2 - font->height / 2 +
                          font->baseline / 2,
                      font, buffer);

    font = &Font::DEFAULT;
    textWidth = font->GetStringWidth("wpm");
    Ssd1306::DrawText(displayId, JAVELIN_OLED_WIDTH / 2 - textWidth / 2,
                      JAVELIN_OLED_HEIGHT * 3 / 4 + font->baseline / 2, font,
                      "wpm");
  }
};

void StenoStrokeCapture::SetAutoDraw_Binding(void *context,
                                             const char *commandLine) {
  StenoStrokeCapture *capture = (StenoStrokeCapture *)context;
  const char *p = strchr(commandLine, ' ');
  if (!p) {
    Console::Printf("ERR No parameters specified\n\n");
    return;
  }
  int displayId = 0;
  ++p;
#if !JAVELIN_SPLIT
  if ('a' <= *p && *p <= 'z')
    goto skipDisplayId;
#endif
  if (*p < '0' && *p >= '9') {
    Console::Printf("ERR displayId parameter missing\n\n");
    return;
  }
  while ('0' <= *p && *p <= '9') {
    displayId = 10 * displayId + *p - '0';
    ++p;
  }
  if (*p != ' ') {
    Console::Printf("ERR displayId parameter missing\n\n");
    return;
  }
  ++p;

skipDisplayId:
  if (Str::Eq(p, "none")) {
    capture->SetAutoDraw(displayId, AutoDraw::NONE);
  } else if (Str::Eq(p, "paper_tape")) {
    capture->SetAutoDraw(displayId, AutoDraw::PAPER_TAPE);
  } else if (Str::Eq(p, "steno_layout")) {
    capture->SetAutoDraw(displayId, AutoDraw::STENO_LAYOUT);
  } else if (Str::Eq(p, "wpm")) {
    capture->SetAutoDraw(displayId, AutoDraw::WPM);
  } else {
    Console::Printf("ERR Unable to set auto draw: \"%s\"\n\n", p);
    return;
  }
  Console::SendOk();
}

void MeasureText_Binding(void *context, const char *commandLine) {
  const char *p = strchr(commandLine, ' ');

  if (!p) {
    Console::Printf("ERR No parameters specified\n\n");
    return;
  }
  int fontId = 0;
  ++p;
  if (*p < '0' && *p >= '9') {
    Console::Printf("ERR fontId parameter missing\n\n");
    return;
  }
  while ('0' <= *p && *p <= '9') {
    fontId = 10 * fontId + *p - '0';
    ++p;
  }
  if (*p != ' ') {
    Console::Printf("ERR fontId parameter missing\n\n");
    return;
  }
  ++p;

  // TODO: Use fontId.
  uint32_t width = Font::DEFAULT.GetStringWidth(p);
  Console::Printf("Width: %u\n\n", width);
}

#endif

#if JAVELIN_USE_EMBEDDED_STENO
#if JAVELIN_USE_USER_DICTIONARY
StenoUserDictionaryData
    userDictionaryLayout((const uint8_t *)STENO_USER_DICTIONARY_ADDRESS,
                         STENO_USER_DICTIONARY_SIZE);
static JavelinStaticAllocate<StenoUserDictionary> userDictionaryContainer;
#endif
#endif

static StenoGemini gemini;
static StenoPloverHid ploverHid;

static JavelinStaticAllocate<StenoDictionaryList> dictionaryListContainer;
#if JAVELIN_USE_EMBEDDED_STENO
static JavelinStaticAllocate<StenoEngine> engineContainer;
#endif
#if JAVELIN_OLED_DRIVER
static JavelinStaticAllocate<StenoStrokeCapture> passthroughContainer;
#else
static JavelinStaticAllocate<StenoPassthrough> passthroughContainer;
#endif
static JavelinStaticAllocate<StenoFirstUp> firstUpContainer;
static JavelinStaticAllocate<StenoAllUp> allUpContainer;
static JavelinStaticAllocate<StenoRepeat> repeatContainer;
static JavelinStaticAllocate<StenoJeffModifiers> jeffModifiersContainer;
static StenoProcessorElement *processors;

static JavelinStaticAllocate<StenoCompiledOrthography>
    compiledOrthographyContainer;
static JavelinStaticAllocate<StenoReverseMapDictionary>
    reverseMapDictionaryContainer;
static JavelinStaticAllocate<StenoReversePrefixDictionary>
    reversePrefixDictionaryContainer;
static JavelinStaticAllocate<StenoReverseAutoSuffixDictionary>
    reverseAutoSuffixDictionaryContainer;

static List<StenoDictionaryListEntry> dictionaries;

extern int resumeCount;
extern "C" char __data_start__[], __data_end__[];
extern "C" char __bss_start__[], __bss_end__[];

#if JAVELIN_USE_EMBEDDED_STENO
static void PrintInfo_Binding(void *context, const char *commandLine) {
  StenoEngine *engine = (StenoEngine *)context;
  const StenoConfigBlock *config =
      (const StenoConfigBlock *)STENO_CONFIG_BLOCK_ADDRESS;

  uint32_t uptime = Clock::GetCurrentTime();
  auto &uptimeDivider = divider->Divide(uptime, 1000);
  uint32_t microseconds = uptimeDivider.remainder;
  uint32_t totalSeconds = uptimeDivider.quotient;
  auto &totalSecondsDivider = divider->Divide(totalSeconds, 60);
  uint32_t seconds = totalSecondsDivider.remainder;
  uint32_t totalMinutes = totalSecondsDivider.quotient;
  auto &totalMinutesDivider = divider->Divide(totalMinutes, 60);
  uint32_t minutes = totalMinutesDivider.remainder;
  uint32_t totalHours = totalMinutesDivider.quotient;
  auto &totalHoursDivider = divider->Divide(totalHours, 24);
  uint32_t hours = totalHoursDivider.remainder;
  uint32_t days = totalHoursDivider.quotient;

  Console::Printf("Uptime: %ud %uh %um %0u.%03us\n", days, hours, minutes,
                  seconds, microseconds);
  Console::Printf("Resume count: %d\n", resumeCount);
  Console::Printf("Chip version: %u\n", rp2040_chip_version());
  Console::Printf("ROM version: %u\n", rp2040_rom_version());

  Console::Printf("Serial number: ");
  uint8_t serialId[8];

  uint32_t interrupts = save_and_disable_interrupts();
  flash_get_unique_id(serialId);
  restore_interrupts(interrupts);

  for (size_t i = 0; i < 8; ++i) {
    Console::Printf("%02x", serialId[i]);
  }
  Console::Printf("\n");
  HidKeyboardReportBuilder::instance.PrintInfo();

  uint32_t systemClockKhz = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_SYS);
  uint32_t systemMhz = divider->Divide(systemClockKhz, 1000).quotient;
  uint32_t usbClockKhz = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_USB);
  uint32_t usbMhz = divider->Divide(usbClockKhz, 1000).quotient;

  Console::Printf("Clocks\n");
  Console::Printf("  System: %u MHz\n", systemMhz);
  Console::Printf("  USB: %u MHz\n", usbMhz);

  Console::Printf("Memory\n");
  Console::Printf("  Data: %zu\n", __data_end__ - __data_start__);
  Console::Printf("  BSS: %zu\n", __bss_end__ - __bss_start__);
  struct mallinfo info = mallinfo();
  Console::Printf("  Arena: %zu\n", info.arena);
  Console::Printf("  Free chunks: %zu\n", info.ordblks);
  Console::Printf("  Used: %zu\n", info.uordblks);
  Console::Printf("  Free: %zu\n", info.fordblks);

  Flash::PrintInfo();
  HidReportBufferBase::PrintInfo();
  SplitTxRx::PrintInfo();
  Ssd1306::PrintInfo();
  Console::Printf("Processing chain\n");
  processors->PrintInfo();
  Console::Printf("Text block: %zu bytes\n",
                  STENO_MAP_DICTIONARY_COLLECTION_ADDRESS->textBlockLength);
  Console::Write("\n", 1);
}
#endif

#if JAVELIN_OLED_DRIVER
void Display::SetAutoDraw(int displayId, int autoDrawId) {
  passthroughContainer->SetAutoDraw(displayId, (AutoDraw)autoDrawId);
}
#endif

static void LaunchBootrom(void *const, const char *commandLine) {
  Bootrom::Launch();
}

static void LaunchSlaveBootrom(void *const, const char *commandLine) {
  Bootrom::LaunchSlave();
}

void SetUnicodeMode(void *context, const char *commandLine) {
  const char *mode = strchr(commandLine, ' ');
  if (!mode) {
    Console::Printf("ERR No mode specified\n\n");
    return;
  }
  ++mode;

  if (StenoKeyCodeEmitter::SetUnicodeMode(mode)) {
    Console::SendOk();
  } else {
    Console::Printf("ERR Unable to set mode: \"%s\"\n\n", mode);
  }
}

void SetStenoMode(void *context, const char *commandLine) {
  const char *stenoMode = strchr(commandLine, ' ');
  if (!stenoMode) {
    Console::Printf("ERR No steno mode layout specified\n\n");
    return;
  }

  ++stenoMode;
#if JAVELIN_USE_EMBEDDED_STENO
  if (Str::Eq(stenoMode, "embedded")) {
    passthroughContainer->SetNext(&engineContainer.value);
  } else
#endif
      if (Str::Eq(stenoMode, "gemini")) {
    passthroughContainer->SetNext(&gemini);
  } else if (Str::Eq(stenoMode, "plover_hid")) {
    passthroughContainer->SetNext(&ploverHid);
  } else {
    Console::Printf("ERR Unable to set steno mode: \"%s\"\n\n", stenoMode);
    return;
  }

  Console::SendOk();
}

void SetKeyboardProtocol(void *context, const char *commandLine) {
  const char *keyboardProtocol = strchr(commandLine, ' ');
  if (!keyboardProtocol) {
    Console::Printf("ERR No keyboard protocol specified\n\n");
    return;
  }

  ++keyboardProtocol;
  if (Str::Eq(keyboardProtocol, "default")) {
    HidKeyboardReportBuilder::instance.SetCompatibilityMode(false);
  } else if (Str::Eq(keyboardProtocol, "compatibility")) {
    HidKeyboardReportBuilder::instance.SetCompatibilityMode(true);
  } else {
    Console::Printf("ERR Unable to set keyboard protocol: \"%s\"\n\n",
                    keyboardProtocol);
    return;
  }

  Console::SendOk();
}

#if JAVELIN_USE_EMBEDDED_STENO
void StenoOrthography_Print_Binding(void *context, const char *commandLine) {
  ORTHOGRAPHY_ADDRESS->Print();
}
#endif

#if ENABLE_DEBUG_COMMAND
void Debug_Binding(void *context, const char *commandLine) {}
#endif

#if JAVELIN_USE_WATCHDOG
uint32_t watchdogData[8];
void Watchdog_Binding(void *context, const char *commandLine) {
  Console::Printf("Watchdog scratch\n");
  for (int i = 0; i < 8; ++i) {
    Console::Printf("  %d: %08x\n", i, watchdogData[i]);
  }
  Console::Printf("\n");
}
#endif

struct WordListData {
  uint32_t length;
  uint8_t data[1];
};

void InitSlave() {
  Console &console = Console::instance;
  console.RegisterCommand("launch_slave_bootrom", "Launch slave rp2040 bootrom",
                          LaunchBootrom, nullptr);
#if JAVELIN_USE_WATCHDOG
  console.RegisterCommand("watchdog", "Show watchdog scratch registers",
                          Watchdog_Binding, nullptr);
#endif
#if ENABLE_DEBUG_COMMAND
  console.RegisterCommand("debug", "Runs debug code", Debug_Binding, nullptr);
#endif
}

void InitJavelinSteno() {
  const StenoConfigBlock *config = STENO_CONFIG_BLOCK_ADDRESS;

  HidKeyboardReportBuilder::instance.SetCompatibilityMode(
      config->hidCompatibilityMode);
  StenoKeyCodeEmitter::SetUnicodeMode(config->unicodeMode);
  KeyboardLayout::SetActiveLayout(config->keyboardLayout);

#if JAVELIN_USE_EMBEDDED_STENO
  const WordListData *const wordListData =
      (const WordListData *)STENO_WORD_LIST_ADDRESS;
  WordList::SetData(wordListData->data, wordListData->length);

  memcpy(StenoKeyState::STROKE_BIT_INDEX_LOOKUP, config->keyMap,
         sizeof(config->keyMap));

  // Setup dictionary list.
  // dictionaries.Add(
  //     StenoDictionaryListEntry(&StenoDebugDictionary::instance, true));

#if JAVELIN_USE_USER_DICTIONARY
  StenoUserDictionary *userDictionary =
      new (userDictionaryContainer) StenoUserDictionary(userDictionaryLayout);
  dictionaries.Add(StenoDictionaryListEntry(userDictionary, true));
#endif

  dictionaries.Add(StenoDictionaryListEntry(
      &StenoJeffShowStrokeDictionary::instance, config->useJeffShowStroke));

  dictionaries.Add(StenoDictionaryListEntry(
      &StenoJeffPhrasingDictionary::instance, config->useJeffPhrasing));

  dictionaries.Add(StenoDictionaryListEntry(
      &StenoJeffNumbersDictionary::instance, config->useJeffNumbers));

  dictionaries.Add(StenoDictionaryListEntry(
      &StenoEmilySymbolsDictionary::instance, config->useEmilySymbols));

  if (STENO_MAP_DICTIONARY_COLLECTION_ADDRESS->magic !=
      STENO_MAP_DICTIONARY_COLLECTION_MAGIC) {
    dictionaries.Add(
        StenoDictionaryListEntry(&StenoCorruptedDictionary::instance, true));
  } else {
    for (size_t i = 0;
         i < STENO_MAP_DICTIONARY_COLLECTION_ADDRESS->dictionaryCount; ++i) {
      const StenoMapDictionaryDefinition *definition =
          STENO_MAP_DICTIONARY_COLLECTION_ADDRESS->dictionaries[i];

      dictionaries.Add(StenoDictionaryListEntry(
          new StenoMapDictionary(*definition), definition->defaultEnabled));
    }
  }

  new (dictionaryListContainer) StenoDictionaryList(dictionaries);
  new (compiledOrthographyContainer)
      StenoCompiledOrthography(*ORTHOGRAPHY_ADDRESS);

  StenoDictionary *dictionary = &dictionaryListContainer.value;
  if (STENO_MAP_DICTIONARY_COLLECTION_ADDRESS->hasReverseLookup) {
    dictionary = new (reverseMapDictionaryContainer) StenoReverseMapDictionary(
        dictionary, (const uint8_t *)STENO_MAP_DICTIONARY_COLLECTION_ADDRESS,
        STENO_MAP_DICTIONARY_COLLECTION_ADDRESS->textBlock,
        STENO_MAP_DICTIONARY_COLLECTION_ADDRESS->textBlockLength);

    dictionary = new (reverseAutoSuffixDictionaryContainer)
        StenoReverseAutoSuffixDictionary(dictionary,
                                         compiledOrthographyContainer);

    dictionary =
        new (reversePrefixDictionaryContainer) StenoReversePrefixDictionary(
            dictionary,
            (const uint8_t *)STENO_MAP_DICTIONARY_COLLECTION_ADDRESS,
            STENO_MAP_DICTIONARY_COLLECTION_ADDRESS->textBlock,
            STENO_MAP_DICTIONARY_COLLECTION_ADDRESS->textBlockLength);
  }

  // Set up processors.
  StenoEngine *engine = new (engineContainer)
      StenoEngine(*dictionary, compiledOrthographyContainer,
#if JAVELIN_USE_USER_DICTIONARY
                  userDictionary
#else
                  nullptr
#endif
      );

  engine->SetSpaceAfter(config->useSpaceAfter);
#endif

  Console &console = Console::instance;
#if JAVELIN_USE_EMBEDDED_STENO
  console.RegisterCommand("info", "System information", PrintInfo_Binding,
                          nullptr);
#endif
  console.RegisterCommand("launch_bootrom", "Launch rp2040 bootrom",
                          LaunchBootrom, nullptr);
#if JAVELIN_SPLIT
  console.RegisterCommand("launch_slave_bootrom", "Launch slave rp2040 bootrom",
                          LaunchSlaveBootrom, nullptr);
#endif
#if JAVELIN_USE_WATCHDOG
  console.RegisterCommand("watchdog", "Show watchdog scratch registers",
                          Watchdog_Binding, nullptr);
#endif

#if JAVELIN_USE_EMBEDDED_STENO
  console.RegisterCommand("set_steno_mode",
                          "Sets the current steno mode [\"embedded\", "
                          "\"gemini\", \"plover_hid\"]",
                          SetStenoMode, nullptr);
#endif
  console.RegisterCommand("set_keyboard_protocol",
                          "Sets the current keyboard protocol "
                          "[\"default\", \"compatibility\"]",
                          SetKeyboardProtocol, nullptr);
  console.RegisterCommand("set_unicode_mode", "Sets the current unicode mode",
                          SetUnicodeMode, nullptr);
  console.RegisterCommand("set_keyboard_layout",
                          "Sets the current keyboard layout",
                          &KeyboardLayout::SetKeyboardLayout_Binding, nullptr);
#if JAVELIN_USE_EMBEDDED_STENO
  console.RegisterCommand("set_space_position",
                          "Controls space position before or after",
                          StenoEngine::SetSpacePosition_Binding, engine);
  console.RegisterCommand("list_dictionaries", "Lists dictionaries",
                          StenoEngine::ListDictionaries_Binding, engine);
  console.RegisterCommand("enable_dictionary", "Enables a dictionary",
                          StenoEngine::EnableDictionary_Binding, engine);
  console.RegisterCommand("disable_dictionary", "Disable a dictionary",
                          StenoEngine::DisableDictionary_Binding, engine);
  console.RegisterCommand("toggle_dictionary", "Toggle a dictionary",
                          StenoEngine::ToggleDictionary_Binding, engine);
  console.RegisterCommand(
      "enable_dictionary_status", "Enable sending dictionary status updates",
      StenoDictionaryList::EnableDictionaryStatus_Binding, nullptr);
  console.RegisterCommand(
      "disable_dictionary_status", "Disable sending dictionary status updates",
      StenoDictionaryList::DisableDictionaryStatus_Binding, nullptr);
  console.RegisterCommand("print_dictionary",
                          "Prints all dictionaries in JSON format",
                          StenoEngine::PrintDictionary_Binding, engine);
  console.RegisterCommand("print_orthography",
                          "Prints all orthography rules in JSON format",
                          StenoOrthography_Print_Binding, nullptr);
  console.RegisterCommand("enable_paper_tape", "Enables paper tape output",
                          StenoEngine::EnablePaperTape_Binding, engine);
  console.RegisterCommand("disable_paper_tape", "Disables paper tape output",
                          StenoEngine::DisablePaperTape_Binding, engine);
  console.RegisterCommand("enable_suggestions", "Enables suggestions output",
                          StenoEngine::EnableSuggestions_Binding, engine);
  console.RegisterCommand("disable_suggestions", "Disables suggestions output",
                          StenoEngine::DisableSuggestions_Binding, engine);
  console.RegisterCommand("enable_text_log", "Enables text log output",
                          StenoEngine::EnableTextLog_Binding, engine);
  console.RegisterCommand("disable_text_log", "Disables text log output",
                          StenoEngine::DisableTextLog_Binding, engine);
  console.RegisterCommand("lookup", "Looks up a word",
                          StenoEngine::Lookup_Binding, engine);
  console.RegisterCommand("lookup_stroke", "Looks up a stroke",
                          StenoEngine::LookupStroke_Binding, engine);
#endif

  if (Ws2812::IsAvailable()) {
    console.RegisterCommand("set_rgb", "Sets a single RGB (index, r, g, b)",
                            Rgb::SetRgb_Binding, nullptr);
  }

#if JAVELIN_OLED_DRIVER
  console.RegisterCommand("set_auto_draw",
                          "Set a display auto-draw mode [\"none\", "
                          "\"paper_tape\", \"steno_layout\", \"wpm\"]",
                          StenoStrokeCapture::SetAutoDraw_Binding,
                          &passthroughContainer.value);
  console.RegisterCommand("measure_text", "Measures the width of text",
                          MeasureText_Binding, nullptr);
#endif

#if ENABLE_DEBUG_COMMAND
  console.RegisterCommand("debug", "Runs debug code", Debug_Binding, nullptr);
#endif

#if JAVELIN_USE_EMBEDDED_STENO
#if JAVELIN_USE_USER_DICTIONARY
  console.RegisterCommand(
      "print_user_dictionary", "Prints the user dictionary in JSON format",
      StenoUserDictionary::PrintJsonDictionary_Binding, userDictionary);
  console.RegisterCommand("reset_user_dictionary", "Resets the user dictionary",
                          StenoUserDictionary::Reset_Binding, userDictionary);
  console.RegisterCommand(
      "add_user_entry", "Adds a definition to the user dictionary",
      StenoUserDictionary::AddEntry_Binding, userDictionary);
  console.RegisterCommand(
      "remove_user_entry", "Removes a definition from the user dictionary",
      StenoUserDictionary::RemoveEntry_Binding, userDictionary);
#endif

  StenoProcessorElement *processorElement = engine;
#else
  StenoProcessorElement *processorElement = &gemini;
#endif
#if JAVELIN_OLED_DRIVER
  processorElement =
      new (passthroughContainer) StenoStrokeCapture(processorElement);
#else
  processorElement =
      new (passthroughContainer) StenoPassthrough(processorElement);
#endif

#if JAVELIN_USE_EMBEDDED_STENO
  if (config->useJeffModifiers) {
    processorElement =
        new (jeffModifiersContainer) StenoJeffModifiers(*processorElement);
  }
#endif

  if (config->useFirstUp) {
    processorElement = new (firstUpContainer) StenoFirstUp(*processorElement);
  } else {
    processorElement = new (allUpContainer) StenoAllUp(*processorElement);
  }

  if (config->useRepeat) {
    processorElement = new (repeatContainer) StenoRepeat(*processorElement);
  }

  processors = processorElement;
}

void Script::OnStenoKeyPressed() {
  processors->Process(stenoState, StenoAction::PRESS);
}

void Script::OnStenoKeyReleased() {
#if TRACE_RELEASE_PROCESSING_TIME
  uint32_t t0 = time_us_32();
#endif
  processors->Process(stenoState, StenoAction::RELEASE);
#if TRACE_RELEASE_PROCESSING_TIME
  uint32_t t1 = time_us_32();
  Console::Printf("Release processing time: %u\n\n", t1 - t0);
#endif
}

void Script::OnStenoStateCancelled() {
  processors->Process(StenoKeyState(0), StenoAction::CANCEL);
}

void Script::SendText(const uint8_t *text) const {
#if JAVELIN_USE_EMBEDDED_STENO
  engineContainer->SendText(text);
#endif
}

void ProcessStenoTick() {
  processors->Tick();
  HidKeyboardReportBuilder::instance.FlushIfRequired();
  ConsoleBuffer::instance.Flush();
}

//---------------------------------------------------------------------------

void ConsoleWriter::Write(const char *data, size_t length) {
  ConsoleBuffer::instance.SendData((const uint8_t *)data, length);
}

void Console::Flush() { ConsoleBuffer::instance.Flush(); }

void Key::PressRaw(uint8_t key) {
#if JAVELIN_OLED_DRIVER
  if (key == KeyCode::BACKSPACE) {
    WpmTracker::instance.Tally(-1);
  } else if (IsWritingKeyCode(key)) {
    WpmTracker::instance.Tally(1);
  }
#endif

  HidKeyboardReportBuilder::instance.Press(key);
}

void Key::ReleaseRaw(uint8_t key) {
  HidKeyboardReportBuilder::instance.Release(key);
}

void Key::Flush() { HidKeyboardReportBuilder::instance.Flush(); }

void SerialPort::SendData(const uint8_t *data, size_t length) {
  if (tud_cdc_connected()) {
    tud_cdc_write(data, length);
    tud_cdc_write_flush();
  } else {
#if JAVELIN_SPLIT
    if (SplitTxRx::IsMaster()) {
      SplitSerialBuffer::Add(data, length);
    }
#endif
  }
}

uint32_t Clock::GetCurrentTime() { return (time_us_64() * 4294968) >> 32; }

void StenoPloverHid::SendPacket(const StenoPloverHidPacket &packet) {
  PloverHidReportBuffer::instance.SendReport(
      ITF_NUM_PLOVER_HID, 0x50, (uint8_t *)&packet, sizeof(packet));
}

void Gpio::SetPin(int pin, bool value) {
  gpio_init(pin);
  gpio_set_dir(pin, true);
  gpio_put(pin, value);
}

//---------------------------------------------------------------------------
