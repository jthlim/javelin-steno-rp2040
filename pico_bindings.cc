//---------------------------------------------------------------------------

#include "console_buffer.h"
#include "hid_keyboard_report_builder.h"
#include "hid_report_buffer.h"
#include "javelin/clock.h"
#include "javelin/config_block.h"
#include "javelin/console.h"
#include "javelin/dictionary/dictionary.h"
#include "javelin/dictionary/dictionary_list.h"
#include "javelin/dictionary/emily_symbols_dictionary.h"
#include "javelin/dictionary/jeff_numbers_dictionary.h"
#include "javelin/dictionary/jeff_phrasing_dictionary.h"
#include "javelin/dictionary/jeff_show_stroke_dictionary.h"
#include "javelin/dictionary/main_dictionary.h"
#include "javelin/dictionary/map_dictionary.h"
#include "javelin/dictionary/map_dictionary_definition.h"
#include "javelin/dictionary/user_dictionary.h"
#include "javelin/engine.h"
#include "javelin/key_code.h"
#include "javelin/orthography.h"
#include "javelin/processor/all_up.h"
#include "javelin/processor/first_up.h"
#include "javelin/processor/gemini.h"
#include "javelin/processor/jeff_modifiers.h"
#include "javelin/processor/plover_hid.h"
#include "javelin/processor/processor_list.h"
#include "javelin/processor/repeat.h"
#include "javelin/processor/switch.h"
#include "javelin/serial_port.h"
#include "javelin/steno_key_code.h"
#include "javelin/steno_key_code_emitter.h"
#include "javelin/word_list.h"

#include "uniV4/config.h"
#include "usb_descriptors.h"

#include "hardware/structs/clocks.h"
#include "tusb.h"
#include <hardware/clocks.h>
#include <hardware/timer.h>

//---------------------------------------------------------------------------

Console console;
ConsoleBuffer consoleSendBuffer;
HidKeyboardReportBuilder reportBuilder;

//---------------------------------------------------------------------------

#if USE_USER_DICTIONARY
StenoUserDictionaryData
    userDictionaryLayout((const uint8_t *)STENO_USER_DICTIONARY_ADDRESS,
                         STENO_USER_DICTIONARY_SIZE);

void StenoUserDictionary_PrintJsonDictionary_Binding(void *context,
                                                     const char *commandLine) {
  StenoUserDictionary *userDictionary = (StenoUserDictionary *)context;
  userDictionary->PrintJsonDictionary();
}

void StenoUserDictionary_Reset_Binding(void *context, const char *commandLine) {
  StenoUserDictionary *userDictionary = (StenoUserDictionary *)context;
  userDictionary->Reset();
  Console::Write("OK\n\n", 4);
}
#endif

void *operator new(size_t, void *p) { return p; }

#define MAX_STENO_DICTIONARIES 6
const StenoDictionary *DICTIONARIES[MAX_STENO_DICTIONARIES];
// UserDictionary.
// &StenoJeffShowStrokeDictionary::instance,
// &StenoJeffPhrasingDictionary::instance,
// &StenoJeffNumbersDictionary::instance,
// &StenoEmilySymbolsDictionary::instance,
// &mainDictionary,

struct StenoDictionaryListContainer {
  StenoDictionaryListContainer() {}
  union {
    char buffer[sizeof(StenoDictionaryList)];
    StenoDictionaryList dictionaries;
  };
};

static StenoDictionaryListContainer dictionaryListContainer;
static StenoGemini gemini;
static StenoPloverHid ploverHid;

static constexpr StenoProcessorElement *alternateProcessors[] = {
    &gemini,
    &ploverHid,
};
static StenoProcessorList alternateProcessor(alternateProcessors, 2);

static StenoProcessor *processor;

static void PrintInfo_Binding(void *context, const char *commandLine) {
  StenoEngine *engine = (StenoEngine *)context;
  const StenoConfigBlock *config =
      (const StenoConfigBlock *)STENO_CONFIG_BLOCK_ADDRESS;

  uint32_t uptime = Clock::GetCurrentTime();
  uint32_t microseconds = uptime % 1000;
  uint32_t totalSeconds = uptime / 1000;
  uint32_t seconds = totalSeconds % 60;
  uint32_t totalMinutes = totalSeconds / 60;
  uint32_t minutes = totalMinutes % 60;
  uint32_t totalHours = totalMinutes / 60;
  uint32_t hours = totalHours % 24;
  uint32_t days = totalHours / 24;

  Console::Printf("Uptime: %ud %uh %um %0u.%03us\n", days, hours, minutes,
                  seconds, microseconds);

  uint32_t systemClockKhz = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_SYS);
  uint32_t usbClockKhz = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_USB);

  Console::Printf("Clocks\n");
  Console::Printf("  System: %u MHz\n", systemClockKhz / 1000);
  Console::Printf("  USB: %u MHz\n", usbClockKhz / 1000);
  Flash::PrintInfo();
  HidReportBuffer::PrintInfo();
  processor->PrintInfo();
  Console::Write("\n", 1);
}

void SetUnicodeMode(void *context, const char *commandLine) {
  const char *mode = strchr(commandLine, ' ');
  if (!mode) {
    Console::Printf("ERR No mode specified\n\n");
    return;
  }
  ++mode;

  if (StenoKeyCodeEmitter::SetUnicodeMode(mode)) {
    Console::Write("OK\n\n", 4);
  } else {
    Console::Printf("ERR Unable to set mode: \"%s\"\n\n", mode);
  }
}

void SetKeyboardLayout(void *context, const char *commandLine) {
  const char *keyboardLayout = strchr(commandLine, ' ');
  if (!keyboardLayout) {
    Console::Printf("ERR No keyboard layout specified\n\n");
    return;
  }
  ++keyboardLayout;

  if (Key::SetKeyboardLayout(keyboardLayout)) {
    Console::Write("OK\n\n", 4);
  } else {
    Console::Printf("ERR Unable to set keyboard layout: \"%s\"\n\n",
                    keyboardLayout);
  }
}

void StenoEngine_PrintDictionary_Binding(void *context,
                                         const char *commandLine) {
  StenoEngine *engine = (StenoEngine *)context;
  engine->PrintDictionary();
}

void StenoOrthography_Print_Binding(void *context, const char *commandLine) {
  ORTHOGRAPHY_ADDRESS->Print();
}

struct WordListData {
  uint32_t length;
  uint8_t data[1];
};

void InitJavelinSteno() {
  const StenoConfigBlock *config =
      (const StenoConfigBlock *)STENO_CONFIG_BLOCK_ADDRESS;

  StenoKeyCodeEmitter::SetUnicodeMode(config->unicodeMode);
  Key::SetKeyboardLayout(config->keyboardLayout);

  const WordListData *const wordListData =
      (const WordListData *)STENO_WORD_LIST_ADDRESS;
  WordList::SetData(wordListData->data, wordListData->length);

  memcpy(StenoKeyState::CHORD_BIT_INDEX_LOOKUP, config->keyMap,
         sizeof(config->keyMap));

  // Setup dictionary list.
  const StenoDictionary **p = DICTIONARIES;

#if USE_USER_DICTIONARY
  StenoUserDictionary *userDictionary =
      new StenoUserDictionary(userDictionaryLayout);
  *p++ = userDictionary;
#endif

  if (config->useJeffShowStroke) {
    *p++ = &StenoJeffShowStrokeDictionary::instance;
  }
  if (config->useJeffPhrasing) {
    *p++ = &StenoJeffPhrasingDictionary::instance;
  }
  if (config->useJeffNumbers) {
    *p++ = &StenoJeffNumbersDictionary::instance;
  }
  if (config->useEmilySymbols) {
    *p++ = &StenoEmilySymbolsDictionary::instance;
  }
  *p++ = new StenoMapDictionary(
      *(const StenoMapDictionaryDefinition *)STENO_MAIN_DICTIONARY_ADDRESS);

  new (dictionaryListContainer.buffer)
      StenoDictionaryList(DICTIONARIES, p - DICTIONARIES);

  // Set up processors.
  StenoEngine *engine = new StenoEngine(dictionaryListContainer.dictionaries,
                                        *ORTHOGRAPHY_ADDRESS,
#if USE_USER_DICTIONARY
                                        userDictionary
#else
                                        nullptr
#endif
  );

  console.RegisterCommand("info", "System information", PrintInfo_Binding,
                          nullptr);
  console.RegisterCommand("set_unicode_mode", "Sets the current unicode mode",
                          SetUnicodeMode, nullptr);
  console.RegisterCommand("set_keyboard_layout",
                          "Sets the current keyboard layout", SetKeyboardLayout,
                          nullptr);
  console.RegisterCommand("print_dictionary",
                          "Prints all dictionaries in JSON format",
                          StenoEngine_PrintDictionary_Binding, engine);
  console.RegisterCommand("print_orthography",
                          "Prints all orthography rules in JSON format",
                          StenoOrthography_Print_Binding, nullptr);

#if USE_USER_DICTIONARY
  console.RegisterCommand(
      "print_user_dictionary", "Prints the user dictionary in JSON format",
      StenoUserDictionary_PrintJsonDictionary_Binding, userDictionary);
  console.RegisterCommand("reset_user_dictionary", "Resets the user dictionary",
                          StenoUserDictionary_Reset_Binding, userDictionary);
#endif
  StenoProcessorElement *processorElement =
      new StenoSwitch(*engine, alternateProcessor);

  if (config->useJeffModifiers) {
    processorElement = new JeffModifiers(*processorElement);
  }

  if (config->useFirstUp) {
    processorElement = new StenoFirstUp(*processorElement);
  } else {
    processorElement = new StenoAllUp(*processorElement);
  }

  if (config->useRepeat) {
    processorElement = new StenoRepeat(*processorElement);
  }

  processor = new StenoProcessor(*processorElement);
}

void ProcessStenoKeyState(StenoKeyState keyState) {
  processor->Process(keyState);
}

void ProcessStenoTick() {
  processor->Tick();
  reportBuilder.Flush();
}

//---------------------------------------------------------------------------

void OnConsoleReceiveData(const uint8_t *data, uint8_t length) {
  console.HandleInput((char *)data, length);
  consoleSendBuffer.Flush();
}

//---------------------------------------------------------------------------

void Console::Write(const char *data, size_t length) {
  consoleSendBuffer.SendData((const uint8_t *)data, length);
}

void Key::PressRaw(uint8_t key) { reportBuilder.Press(key); }

void Key::ReleaseRaw(uint8_t key) { reportBuilder.Release(key); }

void SerialPort::SendData(const uint8_t *data, size_t length) {
  tud_cdc_write(data, length);
  tud_cdc_write_flush();
}

uint32_t Clock::GetCurrentTime() { return time_us_64() / 1000; }

void StenoPloverHid::SendPacket(const StenoPloverHidPacket &packet) {
  HidKeyboardReportBuilder::reportBuffer.SendReport(
      ITF_NUM_PLOVER_HID, 0x50, (uint8_t *)&packet, sizeof(packet));
}

//---------------------------------------------------------------------------
