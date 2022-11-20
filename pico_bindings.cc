//---------------------------------------------------------------------------

#include "console_buffer.h"
#include "hid_keyboard_report_builder.h"
#include "hid_report_buffer.h"
#include "javelin/clock.h"
#include "javelin/config_block.h"
#include "javelin/console.h"
#include "javelin/dictionary/corrupted_dictionary.h"
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
#include "javelin/static_allocate.h"
#include "javelin/steno_key_code.h"
#include "javelin/steno_key_code_emitter.h"
#include "javelin/word_list.h"
#include "plover_hid_report_buffer.h"

#include "uniV4/config.h"
#include "usb_descriptors.h"

#include <hardware/clocks.h>
#include <hardware/timer.h>
#include <malloc.h>
#include <pico/platform.h>
#include <tusb.h>

//---------------------------------------------------------------------------

Console console;
ConsoleBuffer consoleSendBuffer;
HidKeyboardReportBuilder reportBuilder;

//---------------------------------------------------------------------------

#if USE_USER_DICTIONARY
StenoUserDictionaryData
    userDictionaryLayout((const uint8_t *)STENO_USER_DICTIONARY_ADDRESS,
                         STENO_USER_DICTIONARY_SIZE);
static JavelinStaticAllocate<StenoUserDictionary> userDictionaryContainer;
#endif

static StenoGemini gemini;
static StenoPloverHid ploverHid;

#if USE_PLOVER_HID
static constexpr StenoProcessorElement *alternateProcessors[] = {
    &gemini,
    &ploverHid,
};
static StenoProcessorList alternateProcessor(alternateProcessors, 2);
#endif

static JavelinStaticAllocate<StenoDictionaryList> dictionaryListContainer;
static JavelinStaticAllocate<StenoEngine> engineContainer;
static JavelinStaticAllocate<StenoSwitch> switchContainer;
static JavelinStaticAllocate<StenoFirstUp> firstUpContainer;
static JavelinStaticAllocate<StenoAllUp> allUpContainer;
static JavelinStaticAllocate<StenoRepeat> repeatContainer;
static JavelinStaticAllocate<StenoJeffModifiers> jeffModifiersContainer;
static JavelinStaticAllocate<StenoProcessor> processorContainer;

static JavelinStaticAllocate<StenoCompiledOrthography>
    compiledOrthographyContainer;
static JavelinStaticAllocate<StenoReverseMapDictionary>
    reverseMapDictionaryContainer;
static JavelinStaticAllocate<StenoReverseAutoSuffixDictionary>
    reverseAutoSuffixDictionaryContainer;

static List<StenoDictionaryListEntry> dictionaries;

extern int resumeCount;

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
  Console::Printf("Resume count: %d\n", resumeCount);
  Console::Printf("Chip version: %u\n", rp2040_chip_version());
  Console::Printf("ROM version: %u\n", rp2040_rom_version());

  uint32_t systemClockKhz = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_SYS);
  uint32_t usbClockKhz = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_USB);

  Console::Printf("Clocks\n");
  Console::Printf("  System: %u MHz\n", systemClockKhz / 1000);
  Console::Printf("  USB: %u MHz\n", usbClockKhz / 1000);

  struct mallinfo info = mallinfo();
  Console::Printf("Memory\n");
  Console::Printf("  Arena: %zu\n", info.arena);
  Console::Printf("  Free chunks: %zu\n", info.ordblks);
  Console::Printf("  Used: %zu\n", info.uordblks);
  Console::Printf("  Free: %zu\n", info.fordblks);

  Flash::PrintInfo();
  HidReportBufferBase::PrintInfo();
  processorContainer->PrintInfo();
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

void StenoOrthography_Print_Binding(void *context, const char *commandLine) {
  ORTHOGRAPHY_ADDRESS->Print();
}

struct WordListData {
  uint32_t length;
  uint8_t data[1];
};

void InitJavelinSteno() {
  const StenoConfigBlock *config = STENO_CONFIG_BLOCK_ADDRESS;

  HidKeyboardReportBuilder::SetCompatibilityMode(config->hidCompatibilityMode);
  StenoKeyCodeEmitter::SetUnicodeMode(config->unicodeMode);
  Key::SetKeyboardLayout(config->keyboardLayout);

  const WordListData *const wordListData =
      (const WordListData *)STENO_WORD_LIST_ADDRESS;
  WordList::SetData(wordListData->data, wordListData->length);

  memcpy(StenoKeyState::CHORD_BIT_INDEX_LOOKUP, config->keyMap,
         sizeof(config->keyMap));

  // Setup dictionary list.
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

  new (compiledOrthographyContainer)
      StenoCompiledOrthography(*ORTHOGRAPHY_ADDRESS);

  if (STENO_MAP_DICTIONARY_COLLECTION_ADDRESS->hasReverseLookup) {
    dictionary = new (reverseMapDictionaryContainer) StenoReverseMapDictionary(
        dictionary, (const uint8_t *)STENO_MAP_DICTIONARY_COLLECTION_ADDRESS,
        STENO_MAP_DICTIONARY_COLLECTION_ADDRESS->textBlock,
        STENO_MAP_DICTIONARY_COLLECTION_ADDRESS->textBlockLength);

    dictionary = new (reverseAutoSuffixDictionaryContainer)
        StenoReverseAutoSuffixDictionary(dictionary,
                                         compiledOrthographyContainer);
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

  console.RegisterCommand("info", "System information", PrintInfo_Binding,
                          nullptr);
  console.RegisterCommand("launch_bootrom", "Launch rp2040 bootrom",
                          LaunchBootrom, nullptr);
  console.RegisterCommand("set_unicode_mode", "Sets the current unicode mode",
                          SetUnicodeMode, nullptr);
  console.RegisterCommand("set_keyboard_layout",
                          "Sets the current keyboard layout", SetKeyboardLayout,
                          nullptr);
  console.RegisterCommand("list_dictionaries", "Lists dictionaries",
                          StenoEngine::ListDictionaries_Binding, engine);
  console.RegisterCommand("enable_dictionary", "Enables a dictionary",
                          StenoEngine::EnableDictionary_Binding, engine);
  console.RegisterCommand("disable_dictionary", "Disable a dictionary",
                          StenoEngine::DisableDictionary_Binding, engine);
  console.RegisterCommand("toggle_dictionary", "Toggle a dictionary",
                          StenoEngine::ToggleDictionary_Binding, engine);
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
  console.RegisterCommand("lookup", "Looks up a word",
                          StenoEngine::Lookup_Binding, engine);

#if USE_USER_DICTIONARY
  console.RegisterCommand(
      "print_user_dictionary", "Prints the user dictionary in JSON format",
      StenoUserDictionary::PrintJsonDictionary_Binding, userDictionary);
  console.RegisterCommand("reset_user_dictionary", "Resets the user dictionary",
                          StenoUserDictionary::Reset_Binding, userDictionary);
#endif
  StenoProcessorElement *processorElement =
#if USE_PLOVER_HID
      new (switchContainer) StenoSwitch(*engine, alternateProcessor);
#else
      new (switchContainer) StenoSwitch(*engine, gemini);
#endif

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

  new (processorContainer) StenoProcessor(*processorElement);
}

void ProcessStenoKeyState(StenoKeyState keyState) {
  processorContainer->Process(keyState);
}

void ProcessStenoTick() {
  processorContainer->Tick();
  reportBuilder.FlushIfRequired();
  consoleSendBuffer.Flush();
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

void Console::Flush() { consoleSendBuffer.Flush(); }

void Key::PressRaw(uint8_t key) { reportBuilder.Press(key); }

void Key::ReleaseRaw(uint8_t key) { reportBuilder.Release(key); }

void SerialPort::SendData(const uint8_t *data, size_t length) {
  tud_cdc_write(data, length);
  tud_cdc_write_flush();
}

uint32_t Clock::GetCurrentTime() { return time_us_64() / 1000; }

#if USE_PLOVER_HID
void StenoPloverHid::SendPacket(const StenoPloverHidPacket &packet) {
  PloverHidReportBuffer::instance.SendReport(
      ITF_NUM_PLOVER_HID, 0x50, (uint8_t *)&packet, sizeof(packet));
}
#endif

//---------------------------------------------------------------------------
