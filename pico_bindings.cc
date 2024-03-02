//---------------------------------------------------------------------------

#include "auto_draw.h"
#include "console_report_buffer.h"
#include "hid_keyboard_report_builder.h"
#include "hid_report_buffer.h"
#include "javelin/clock.h"
#include "javelin/config_block.h"
#include "javelin/console.h"
#include "javelin/dictionary/corrupted_dictionary.h"
#include "javelin/dictionary/debug_dictionary.h"
#include "javelin/dictionary/dictionary.h"
#include "javelin/dictionary/dictionary_definition.h"
#include "javelin/dictionary/dictionary_list.h"
#include "javelin/dictionary/reverse_auto_suffix_dictionary.h"
#include "javelin/dictionary/reverse_map_dictionary.h"
#include "javelin/dictionary/reverse_prefix_dictionary.h"
#include "javelin/dictionary/unicode_dictionary.h"
#include "javelin/dictionary/user_dictionary.h"
#include "javelin/engine.h"
#include "javelin/font/monochrome/font.h"
#include "javelin/hal/bootloader.h"
#include "javelin/hal/connection.h"
#include "javelin/hal/display.h"
#include "javelin/hal/rgb.h"
#include "javelin/key.h"
#include "javelin/orthography.h"
#include "javelin/processor/all_up.h"
#include "javelin/processor/first_up.h"
#include "javelin/processor/gemini.h"
#include "javelin/processor/jeff_modifiers.h"
#include "javelin/processor/passthrough.h"
#include "javelin/processor/plover_hid.h"
#include "javelin/processor/procat.h"
#include "javelin/processor/processor_list.h"
#include "javelin/processor/repeat.h"
#include "javelin/processor/tx_bolt.h"
#include "javelin/script_byte_code.h"
#include "javelin/script_manager.h"
#include "javelin/split/pair_console.h"
#include "javelin/static_allocate.h"
#include "javelin/steno_key_code.h"
#include "javelin/steno_key_code_emitter.h"
#include "javelin/word_list.h"
#include "javelin/wpm_tracker.h"
#include "plover_hid_report_buffer.h"
#include "rp2040_divider.h"
#include "rp2040_split.h"
#include "ssd1306.h"
#include "usb_descriptors.h"

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
#define ENABLE_EXTRA_INFO 0

//---------------------------------------------------------------------------

#if JAVELIN_USE_EMBEDDED_STENO
#if JAVELIN_USE_USER_DICTIONARY
StenoUserDictionaryData
    userDictionaryLayout((const uint8_t *)STENO_USER_DICTIONARY_ADDRESS,
                         STENO_USER_DICTIONARY_SIZE);
static JavelinStaticAllocate<StenoUserDictionary> userDictionaryContainer;
#endif
#endif

static StenoGemini gemini;
static StenoTxBolt txBolt;
static StenoProcat procat;
static StenoPloverHid ploverHid;
static StenoProcessorElement *processors;

#if JAVELIN_DISPLAY_DRIVER
static JavelinStaticAllocate<StenoStrokeCapture> &passthroughContainer =
    StenoStrokeCapture::container;
#else
static JavelinStaticAllocate<StenoPassthrough> passthroughContainer;
#endif

#if JAVELIN_USE_EMBEDDED_STENO
static JavelinStaticAllocate<StenoDictionaryList> dictionaryListContainer;
static JavelinStaticAllocate<StenoEngine> engineContainer;
static JavelinStaticAllocate<StenoJeffModifiers> jeffModifiersContainer;

static JavelinStaticAllocate<StenoCompiledOrthography>
    compiledOrthographyContainer;
static JavelinStaticAllocate<StenoReverseMapDictionary>
    reverseMapDictionaryContainer;
static JavelinStaticAllocate<StenoReversePrefixDictionary>
    reversePrefixDictionaryContainer;
static JavelinStaticAllocate<StenoReverseAutoSuffixDictionary>
    reverseAutoSuffixDictionaryContainer;

static List<StenoDictionaryListEntry> dictionaries;
#endif

static JavelinStaticAllocate<StenoFirstUp> firstUpContainer;
static JavelinStaticAllocate<StenoAllUp> allUpContainer;
static JavelinStaticAllocate<StenoRepeat> repeatContainer;
static JavelinStaticAllocate<StenoPassthrough> triggerContainer;

extern "C" char __data_start__[], __data_end__[];
extern "C" char __bss_start__[], __bss_end__[];

static void PrintInfo_Binding(void *context, const char *commandLine) {
  uint32_t uptime = Clock::GetMilliseconds();
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

  Console::Printf("System\n");
  uint32_t systemClockKhz = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_SYS);
  uint32_t systemMhz = divider->Divide(systemClockKhz, 1000).quotient;
  Console::Printf("  Clock: %u MHz\n", systemMhz);
  Console::Printf("  Uptime: %ud %uh %um %0u.%03us\n", days, hours, minutes,
                  seconds, microseconds);
  Console::Printf("  Chip version: %u\n", rp2040_chip_version());
  Console::Printf("  ROM version: %u\n", rp2040_rom_version());

  Console::Printf("  Serial number: ");
  uint8_t serialId[8];

  uint32_t interrupts = save_and_disable_interrupts();
  flash_get_unique_id(serialId);
  restore_interrupts(interrupts);

  for (size_t i = 0; i < 8; ++i) {
    Console::Printf("%02x", serialId[i]);
  }
  Console::Printf("\n");
  Console::Printf("  Firmware: " __DATE__ " \n");
  Console::Printf("  ");
  HidKeyboardReportBuilder::instance.PrintInfo();

#if ENABLE_EXTRA_INFO
  Connection::PrintInfo();
#endif

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
  Rp2040Split::PrintInfo();

#if ENABLE_EXTRA_INFO
  ScriptManager::GetInstance().PrintInfo();
#endif

  if (Rp2040Split::IsMaster()) {
#if ENABLE_EXTRA_INFO
    // The slave will always just print "Screen: present, present" here.
    // Rather than print incorrect info, don't print anything on the slave at
    // all.
    Ssd1306::PrintInfo();
#endif

    Console::Printf("Processing chain\n");
    processors->PrintInfo();
#if JAVELIN_USE_EMBEDDED_STENO
    Console::Printf("Text block: %zu bytes\n",
                    STENO_MAP_DICTIONARY_COLLECTION_ADDRESS->textBlockLength);
#endif
  }
  Console::Write("\n", 1);
}

void Restart_Binding(void *context, const char *commandLine) {
  watchdog_reboot(0, 0, 0);
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
    Console::Printf("ERR No steno mode specified\n\n");
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
  } else if (Str::Eq(stenoMode, "tx_bolt")) {
    passthroughContainer->SetNext(&txBolt);
  } else if (Str::Eq(stenoMode, "procat")) {
    passthroughContainer->SetNext(&procat);
  } else if (Str::Eq(stenoMode, "plover_hid")) {
    passthroughContainer->SetNext(&ploverHid);
  } else {
    Console::Printf("ERR Unable to set steno mode: \"%s\"\n\n", stenoMode);
    return;
  }

  Console::SendOk();
  ScriptManager::ExecuteScript(ScriptId::STENO_MODE_UPDATE);
}

void SetStenoTrigger(void *context, const char *commandLine) {
  const char *trigger = strchr(commandLine, ' ');
  if (!trigger) {
    Console::Printf("ERR No trigger specified\n\n");
    return;
  }

  ++trigger;
  if (Str::Eq(trigger, "all_up")) {
    triggerContainer->SetNext(&allUpContainer.value);
  } else if (Str::Eq(trigger, "first_up")) {
    triggerContainer->SetNext(&firstUpContainer.value);
  } else {
    Console::Printf("ERR Unable to set trigger mode: \"%s\"\n\n", trigger);
    return;
  }

  Console::SendOk();
  ScriptManager::ExecuteScript(ScriptId::STENO_MODE_UPDATE);
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

struct ParameterData {
  const char *name;
  const void *value;
};

struct DynamicParameterData {
  const char *name;
  void (*const valueProvider)();
};

static const ParameterData PARAMETER_DATA[] = {
    {"button_count", (void *)BUTTON_COUNT},
    {"button_script_address", SCRIPT_BYTE_CODE},
    {"button_script_byte_code_revision", (void *)SCRIPT_BYTE_CODE_REVISION},
#if JAVELIN_USE_EMBEDDED_STENO
    {"dictionary_address", STENO_MAP_DICTIONARY_COLLECTION_ADDRESS},
#endif
    {"flash_memory_address", (void *)0x10000000},
    {"maximum_button_script_size", (void *)MAXIMUM_BUTTON_SCRIPT_SIZE},
#if JAVELIN_USE_EMBEDDED_STENO
    {"maximum_dictionary_size", (void *)MAXIMUM_MAP_DICTIONARY_SIZE},
#endif
};

#if JAVELIN_USE_EMBEDDED_STENO
static void GetKeyboardLayout() {
  Console::Printf("%s\n\n", KeyboardLayout::GetActiveLayout().GetName());
}

static void GetKeyboardProtocol() {
  Console::Printf("%s\n\n",
                  HidKeyboardReportBuilder::instance.IsCompatibilityMode()
                      ? "compatibility"
                      : "default");
}
#endif

#if defined(JAVELIN_SCRIPT_CONFIGURATION)
static void GetScriptConfiguration() {
  Console::Printf("%s\n\n", JAVELIN_SCRIPT_CONFIGURATION);
}
#endif

// clang-format off
#define STR2(x) #x
#define STR(x) STR2(x)
static void GetScriptHeader() {
  Console::Printf(
#if JAVELIN_RGB
  #define HAS_SCRIPT_HEADER 1
      "const JAVELIN_HAS_RGB = 1;\n"
#endif
#if JAVELIN_DISPLAY_DRIVER
  #define HAS_SCRIPT_HEADER 1
      "const JAVELIN_HAS_DISPLAY = 1;\n"
      "const JAVELIN_DISPLAY_WIDTH = " STR(JAVELIN_DISPLAY_WIDTH) ";\n" //
      "const JAVELIN_DISPLAY_HEIGHT = " STR(JAVELIN_DISPLAY_HEIGHT) ";\n" //
#endif
#if HAS_SCRIPT_HEADER
      "\n"
#else
      "\n\n"
#endif
  );
}
// clang-format on

#if JAVELIN_USE_EMBEDDED_STENO
static void GetSpacePosition() {
  Console::Printf("%s\n\n",
                  engineContainer->IsSpaceAfter() ? "after" : "before");
}

static void GetStrokeCount() {
  Console::Printf("%zu\n\n", engineContainer->GetStrokeCount());
}

static void GetUnicodeMode() {
  Console::Printf("%s\n\n", StenoKeyCodeEmitter::GetUnicodeModeName());
}
#endif

static void GetStenoMode() {
  const StenoProcessorElement *processor = passthroughContainer->GetNext();
#if JAVELIN_USE_EMBEDDED_STENO
  if (processor == &engineContainer.value) {
    Console::Printf("embedded\n\n");
  } else
#endif
      if (processor == &gemini) {
    Console::Printf("gemini\n\n");
  } else if (processor == &txBolt) {
    Console::Printf("tx_bolt\n\n");
  } else if (processor == &procat) {
    Console::Printf("procat\n\n");
  } else if (processor == &ploverHid) {
    Console::Printf("plover_hid\n\n");
  } else {
    Console::Printf("ERR Internal consistency error\n\n");
  }
}

static void GetStenoTrigger() {
  const StenoProcessorElement *trigger = triggerContainer->GetNext();
  if (trigger == &allUpContainer.value) {
    Console::Printf("all_up\n\n");
  } else if (trigger == &firstUpContainer.value) {
    Console::Printf("first_up\n\n");
  } else {
    Console::Printf("ERR Internal consistency error\n\n");
  }
}

static const DynamicParameterData DYNAMIC_PARAMETER_DATA[] = {
#if JAVELIN_USE_EMBEDDED_STENO
    {"keyboard_layout", GetKeyboardLayout},
    {"keyboard_protocol", GetKeyboardProtocol},
#endif
#if defined(JAVELIN_SCRIPT_CONFIGURATION)
    {"script_configuration", GetScriptConfiguration},
#endif
    {"script_header", GetScriptHeader},
#if JAVELIN_USE_EMBEDDED_STENO
    {"space_position", GetSpacePosition},
#endif
    {"steno_mode", GetStenoMode},
    {"steno_trigger", GetStenoTrigger},
#if JAVELIN_USE_EMBEDDED_STENO
    {"stroke_count", GetStrokeCount},
    {"unicode_mode", GetUnicodeMode},
#endif
};

void GetParameterBinding(void *context, const char *commandLine) {
  const char *parameterName = strchr(commandLine, ' ');
  if (!parameterName) {
    Console::Printf("ERR No parameter specified\n\n");
    return;
  }
  ++parameterName;

  for (const ParameterData &data : PARAMETER_DATA) {
    if (Str::Eq(parameterName, data.name)) {
      Console::Printf("%p\n\n", data.value);
      return;
    }
  }

  for (const DynamicParameterData &data : DYNAMIC_PARAMETER_DATA) {
    if (Str::Eq(parameterName, data.name)) {
      data.valueProvider();
      return;
    }
  }

  Console::Printf("ERR Unknown parameter %s\n\n", parameterName);
}

void ListParametersBinding(void *context, const char *commandLine) {
  Console::Printf("Static parameters\n");
  for (const ParameterData &data : PARAMETER_DATA) {
    Console::Printf("  • %s\n", data.name);
  }

  Console::Printf(" \nDynamic parameters\n");
  for (const DynamicParameterData &data : DYNAMIC_PARAMETER_DATA) {
    Console::Printf("  • %s\n", data.name);
  }

  Console::Printf("\n");
}

#if JAVELIN_USE_EMBEDDED_STENO
void StenoOrthography_Print_Binding(void *context, const char *commandLine) {
  ORTHOGRAPHY_ADDRESS->Print();
}
#endif

#if JAVELIN_DISPLAY_DRIVER

void MeasureText_Binding(void *context, const char *commandLine) {
  const char *p = strchr(commandLine, ' ');
  if (!p) {
    Console::Printf("ERR No parameters specified\n\n");
    return;
  }

  int fontId;
  p = Str::ParseInteger(&fontId, p + 1, false);
  if (!p || *p != ' ') {
    Console::Printf("ERR fontId parameter missing\n\n");
    return;
  }
  ++p;

  const Font *font = Font::GetFont(fontId);
  uint32_t width = font->GetStringWidth(p);
  Console::Printf("Width: %u\n\n", width);
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

void InitCommonCommands() {
  Console &console = Console::instance;
  console.RegisterCommand("info", "System information", PrintInfo_Binding,
                          nullptr);
  console.RegisterCommand("restart", "Restart the device", Restart_Binding,
                          nullptr);
  console.RegisterCommand("get_parameter",
                          "Gets the value of the specified parameter",
                          GetParameterBinding, nullptr);
  console.RegisterCommand("list_parameters",
                          "Lists all available parameter names",
                          ListParametersBinding, nullptr);

  Flash::AddConsoleCommands(console);
  Rgb::AddConsoleCommands(console);
  Bootloader::AddConsoleCommands(console);

#if JAVELIN_USE_WATCHDOG
  console.RegisterCommand("watchdog", "Show watchdog scratch registers",
                          Watchdog_Binding, nullptr);
#endif

#if ENABLE_DEBUG_COMMAND
  console.RegisterCommand("debug", "Runs debug code", Debug_Binding, nullptr);
#endif
}

void InitJavelinSlave() { InitCommonCommands(); }

void InitJavelinMaster() {
  const StenoConfigBlock *config = STENO_CONFIG_BLOCK_ADDRESS;

#if JAVELIN_USE_EMBEDDED_STENO
  HidKeyboardReportBuilder::instance.SetCompatibilityMode(
      config->hidCompatibilityMode);
  StenoKeyCodeEmitter::SetUnicodeMode(config->unicodeMode);
  KeyboardLayout::SetActiveLayout(config->keyboardLayout);

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

  STENO_MAP_DICTIONARY_COLLECTION_ADDRESS->AddDictionariesToList(dictionaries);

  dictionaries.Add(
      StenoDictionaryListEntry(&StenoUnicodeDictionary::instance, true));

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

  InitCommonCommands();
  Console &console = Console::instance;

#if JAVELIN_USE_EMBEDDED_STENO
  console.RegisterCommand(
      "set_steno_mode",
      "Sets the current steno mode [\"embedded\", "
      "\"gemini\", \"tx_bolt\", \"procat\", \"plover_hid\"]",
      SetStenoMode, nullptr);
#else
  console.RegisterCommand(
      "set_steno_mode",
      "Sets the current steno mode ["
      "\"gemini\", \"tx_bolt\", \"procat\", \"plover_hid\"]",
      SetStenoMode, nullptr);
#endif
  console.RegisterCommand(
      "set_steno_trigger",
      "Sets the current steno trigger [\"first_up\", \"all_up\"]",
      SetStenoTrigger, nullptr);
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
  ScriptManager::GetInstance().AddConsoleCommands(console);
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
  console.RegisterCommand("process_strokes", "Processes a stroke list",
                          StenoEngine::ProcessStrokes_Binding, engine);
#endif

#if JAVELIN_DISPLAY_DRIVER
  console.RegisterCommand("set_auto_draw",
                          "Set a display auto-draw mode [\"none\", "
                          "\"paper_tape\", \"steno_layout\", \"wpm\"]",
                          StenoStrokeCapture::SetAutoDraw_Binding,
                          &passthroughContainer.value);
  console.RegisterCommand("measure_text", "Measures the width of text",
                          MeasureText_Binding, nullptr);
#endif

#if JAVELIN_USE_EMBEDDED_STENO
  userDictionary->AddConsoleCommands(console);

  StenoProcessorElement *processorElement = engine;
#else
  StenoProcessorElement *processorElement = &gemini;
#endif
#if JAVELIN_DISPLAY_DRIVER
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

  new (firstUpContainer) StenoFirstUp(*processorElement);
  new (allUpContainer) StenoAllUp(*processorElement);
  processorElement = config->useFirstUp
                         ? (StenoProcessorElement *)&firstUpContainer.value
                         : (StenoProcessorElement *)&allUpContainer.value;
  processorElement = new (triggerContainer) StenoPassthrough(processorElement);

  if (config->useRepeat) {
    processorElement = new (repeatContainer) StenoRepeat(*processorElement);
  }

  processors = processorElement;

#if JAVELIN_SPLIT
  console.RegisterCommand("pair", "Runs a command on the pair's console",
                          &PairConsole::PairBinding, nullptr);
#endif
}

void Script::OnStenoKeyPressed() {
  processors->Process(stenoState, StenoAction::PRESS);
}

void Script::OnStenoKeyReleased() {
#if TRACE_RELEASE_PROCESSING_TIME
  uint32_t t0 = Clock::GetMicroseconds();
#endif
  processors->Process(stenoState, StenoAction::RELEASE);
#if TRACE_RELEASE_PROCESSING_TIME
  uint32_t t1 = Clock::GetMicroseconds();
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

bool Script::ProcessScanCode(int scanCode, ScanCodeAction action) {
#if JAVELIN_USE_EMBEDDED_STENO
  uint32_t modifiers = keyState.GetRange(KeyCode::L_CTRL, KeyCode::L_CTRL + 8)
                       << MODIFIER_BIT_SHIFT;
  return engineContainer->ProcessScanCode(scanCode | modifiers, action);
#else
  return false;
#endif
}

void ProcessStenoTick() {
  processors->Tick();
  HidKeyboardReportBuilder::instance.FlushIfRequired();
  ConsoleReportBuffer::instance.Flush();
}

//---------------------------------------------------------------------------

void Key::PressRaw(KeyCode key) {
#if JAVELIN_DISPLAY_DRIVER
  if (key.value == KeyCode::BACKSPACE) {
    WpmTracker::instance.Tally(-1);
  } else if (key.IsVisible()) {
    WpmTracker::instance.Tally(1);
  }
#endif

  HidKeyboardReportBuilder::instance.Press(key.value);
}

void Key::ReleaseRaw(KeyCode key) {
  HidKeyboardReportBuilder::instance.Release(key.value);
}

void Key::Flush() { HidKeyboardReportBuilder::instance.Flush(); }

//---------------------------------------------------------------------------
