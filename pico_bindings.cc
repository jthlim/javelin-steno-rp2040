//---------------------------------------------------------------------------

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
#include "javelin/processor/repeat.h"
#include "javelin/processor/switch.h"
#include "javelin/serial_port.h"
#include "javelin/steno_key_code.h"
#include "javelin/steno_key_code_emitter.h"

#include "uniV4/config.h"
#include "usb_descriptors.h"

#include "tusb.h"
#include <hardware/timer.h>

//---------------------------------------------------------------------------

Console console;
HidReportBuilder reportBuilder;

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
static StenoProcessor *processor;

void StenoEngine_PrintInfo_Binding(void *context, const char *commandLine) {
  StenoEngine *engine = (StenoEngine *)context;
  const StenoConfigBlock *config =
      (const StenoConfigBlock *)STENO_CONFIG_BLOCK_ADDRESS;

  engine->PrintInfo(config);
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

void StenoEngine_PrintDictionary_Binding(void *context,
                                         const char *commandLine) {
  StenoEngine *engine = (StenoEngine *)context;
  engine->PrintDictionary();
}

void InitJavelinSteno() {
  const StenoConfigBlock *config =
      (const StenoConfigBlock *)STENO_CONFIG_BLOCK_ADDRESS;

  StenoKeyCodeEmitter::SetUnicodeMode(config->unicodeMode);

  memcpy(StenoKeyState::CHORD_BIT_INDEX_LOOKUP, config->keyMap,
         sizeof(config->keyMap));

  // Setup dictionary list.
  const StenoDictionary **p = DICTIONARIES;
#if USE_USER_DICTIONARY

  StenoUserDictionary *userDictionary =
      new StenoUserDictionary(userDictionaryLayout);
  *p++ = userDictionary;

  console.RegisterCommand(
      "print_user_dictionary", "Prints the user dictionary in JSON format",
      StenoUserDictionary_PrintJsonDictionary_Binding, userDictionary);
  console.RegisterCommand("reset_user_dictionary", "Resets the user dictionary",
                          StenoUserDictionary_Reset_Binding, userDictionary);
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
                                        StenoOrthography::englishOrthography,
#if USE_USER_DICTIONARY
                                        userDictionary
#else
                                        nullptr
#endif
  );

  console.RegisterCommand("info", "System information",
                          StenoEngine_PrintInfo_Binding, engine);
  console.RegisterCommand("set_unicode_mode", "Sets the current unicode mode",
                          SetUnicodeMode, nullptr);
  console.RegisterCommand("print_dictionary",
                          "Prints all dictionaries in JSON format",
                          StenoEngine_PrintDictionary_Binding, engine);

  StenoProcessorElement *processorElement = new StenoSwitch(*engine, gemini);

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
  reportBuilder.Flush();
  tud_cdc_read_flush();
}

void ProcessStenoTick() { processor->Tick(); }

//---------------------------------------------------------------------------

void OnConsoleReceiveData(const char *data, uint8_t length) {
  console.HandleInput(data, length);
}

//---------------------------------------------------------------------------

void Console::Write(const char *data, size_t length) {
  // 
}

void Key::Press(uint8_t key) {
  reportBuilder.Press(key);
}

void Key::Release(uint8_t key) {
  reportBuilder.Release(key);
}

bool isNumLockOn = false;

void SetIsNumLockOn(bool value) { isNumLockOn = value; }

bool Key::IsNumLockOn() { return isNumLockOn; }

void SerialPort::SendData(const uint8_t *data, size_t length) {
  tud_cdc_write(data, length);
  tud_cdc_write_flush();
}

uint32_t Clock::GetCurrentTime() { return time_us_64 () / 1000; }
