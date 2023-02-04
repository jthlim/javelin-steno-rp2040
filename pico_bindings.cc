//---------------------------------------------------------------------------

#include "console_buffer.h"
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
#include "javelin/engine.h"
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
#include "javelin/serial_port.h"
#include "javelin/static_allocate.h"
#include "javelin/steno_key_code.h"
#include "javelin/steno_key_code_emitter.h"
#include "javelin/word_list.h"
#include "plover_hid_report_buffer.h"
#include "rp2040_divider.h"

#include "usb_descriptors.h"

#include <hardware/clocks.h>
#include <hardware/flash.h>
#include <hardware/timer.h>
#include <malloc.h>
#include <tusb.h>

#include JAVELIN_BOARD_CONFIG

//---------------------------------------------------------------------------

#define TRACE_RELEASE_PROCESSING_TIME 0

//---------------------------------------------------------------------------

#if USE_USER_DICTIONARY
StenoUserDictionaryData
    userDictionaryLayout((const uint8_t *)STENO_USER_DICTIONARY_ADDRESS,
                         STENO_USER_DICTIONARY_SIZE);
static JavelinStaticAllocate<StenoUserDictionary> userDictionaryContainer;
#endif

static StenoGemini gemini;
static StenoPloverHid ploverHid;

static JavelinStaticAllocate<StenoDictionaryList> dictionaryListContainer;
static JavelinStaticAllocate<StenoEngine> engineContainer;
static JavelinStaticAllocate<StenoPassthrough> passthroughContainer;
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
  flash_get_unique_id(serialId);
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
  Console::Printf("Processing chain\n");
  processors->PrintInfo();
  Console::Printf("Text block: %zu bytes\n",
                  STENO_MAP_DICTIONARY_COLLECTION_ADDRESS->textBlockLength);
  Console::Write("\n", 1);
}

static void LaunchBootrom(void *const, const char *commandLine) {
  const uint16_t *const functionTableAddress = (const uint16_t *)0x14;
  size_t (*LookupMethod)(uint32_t table, uint32_t key) =
      (size_t(*)(uint32_t, uint32_t))(size_t)functionTableAddress[2];
  void (*UsbBoot)(int, int) = (void (*)(int, int))LookupMethod(
      functionTableAddress[0], 'U' + 'B' * 256);
  (*UsbBoot)(0, 0);
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

void SetKeyboardLayout(void *context, const char *commandLine) {
  const char *keyboardLayout = strchr(commandLine, ' ');
  if (!keyboardLayout) {
    Console::Printf("ERR No keyboard layout specified\n\n");
    return;
  }
  ++keyboardLayout;

  if (KeyboardLayout::SetActiveLayout(keyboardLayout)) {
    Console::SendOk();
  } else {
    Console::Printf("ERR Unable to set keyboard layout: \"%s\"\n\n",
                    keyboardLayout);
  }
}

void SetStenoMode(void *context, const char *commandLine) {
  const char *stenoMode = strchr(commandLine, ' ');
  if (!stenoMode) {
    Console::Printf("ERR No steno mode layout specified\n\n");
    return;
  }

  ++stenoMode;
  if (Str::Eq(stenoMode, "embedded")) {
    passthroughContainer->SetNext(&engineContainer.value);
  } else if (Str::Eq(stenoMode, "gemini")) {
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

void StenoOrthography_Print_Binding(void *context, const char *commandLine) {
  ORTHOGRAPHY_ADDRESS->Print();
}

// void Debug_Test_Binding(void *context, const char *commandLine) {
// }

struct WordListData {
  uint32_t length;
  uint8_t data[1];
};

void InitJavelinSteno() {
  const StenoConfigBlock *config = STENO_CONFIG_BLOCK_ADDRESS;

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

#if USE_USER_DICTIONARY
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
#if USE_USER_DICTIONARY
                  userDictionary
#else
                  nullptr
#endif
      );

  engine->SetSpaceAfter(config->useSpaceAfter);

  Console &console = Console::instance;
  console.RegisterCommand("info", "System information", PrintInfo_Binding,
                          nullptr);
  console.RegisterCommand("launch_bootrom", "Launch rp2040 bootrom",
                          LaunchBootrom, nullptr);
  console.RegisterCommand(
      "set_steno_mode",
      "Sets the current steno mode [\"embedded\", \"gemini\", \"plover_hid\"]",
      SetStenoMode, nullptr);
  console.RegisterCommand(
      "set_keyboard_protocol",
      "Sets the current keyboard protocol [\"default\", \"compatibility\"]",
      SetKeyboardProtocol, nullptr);
  console.RegisterCommand("set_unicode_mode", "Sets the current unicode mode",
                          SetUnicodeMode, nullptr);
  console.RegisterCommand("set_keyboard_layout",
                          "Sets the current keyboard layout", SetKeyboardLayout,
                          nullptr);
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
  // console.RegisterCommand("debug_test", "Runs test", Debug_Test_Binding,
  //                         nullptr);

#if USE_USER_DICTIONARY
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
  processorElement =
      new (passthroughContainer) StenoPassthrough(processorElement);

  if (config->useJeffModifiers) {
    processorElement =
        new (jeffModifiersContainer) StenoJeffModifiers(*processorElement);
  }

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
  engineContainer->SendText(text);
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
  HidKeyboardReportBuilder::instance.Press(key);
}

void Key::ReleaseRaw(uint8_t key) {
  HidKeyboardReportBuilder::instance.Release(key);
}

void Key::Flush() { HidKeyboardReportBuilder::instance.Flush(); }

void SerialPort::SendData(const uint8_t *data, size_t length) {
  tud_cdc_write(data, length);
  tud_cdc_write_flush();
}

uint32_t Clock::GetCurrentTime() { return (time_us_64() * 4294968) >> 32; }

void StenoPloverHid::SendPacket(const StenoPloverHidPacket &packet) {
  if (!tud_hid_n_ready(ITF_NUM_PLOVER_HID)) {
    return;
  }

  PloverHidReportBuffer::instance.SendReport(
      ITF_NUM_PLOVER_HID, 0x50, (uint8_t *)&packet, sizeof(packet));
}

//---------------------------------------------------------------------------
