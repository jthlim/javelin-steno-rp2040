cmake_minimum_required(VERSION 3.5)

set(FAMILY "rp2040")
set(ENV{PICO_BOARD} pico_16mb)

include($ENV{PICO_SDK_PATH}/lib/tinyusb/hw/bsp/family_support.cmake)

# gets PROJECT name for the example (e.g. <BOARD>-<DIR_NAME>)
family_get_project_name(PROJECT ${CMAKE_CURRENT_LIST_DIR})

project(${PROJECT})

# Checks this example is valid for the family and initializes the project
family_initialize_project(${PROJECT} ${CMAKE_CURRENT_LIST_DIR})

add_executable(${PROJECT})

# Example source
target_sources(${PROJECT} PUBLIC
  main.cc
  usb_descriptors.cc

  flash.cc
  key_state.cc
  hid_report_buffer.cc
  pico_bindings.cc

  javelin/chord.cc
  javelin/chord_history.cc
  javelin/console.cc
  javelin/crc32.cc
  javelin/dictionary/dictionary.cc
  javelin/dictionary/dictionary_list.cc
  javelin/dictionary/emily_symbols_dictionary.cc
  javelin/dictionary/jeff_numbers_dictionary.cc
  javelin/dictionary/jeff_phrasing_dictionary.cc
  javelin/dictionary/jeff_phrasing_dictionary_data.cc
  javelin/dictionary/jeff_show_stroke_dictionary.cc
  javelin/dictionary/main_dictionary.cc
  javelin/dictionary/map_dictionary.cc
  javelin/dictionary/user_dictionary.cc
  javelin/engine_add_translation_mode.cc
  javelin/engine_normal_mode.cc
  javelin/engine.cc
  javelin/english_orthography.cc
  javelin/flash.cc
  javelin/key_press_parser.cc
  javelin/list.cc
  javelin/macos_us_unicode_data.cc
  javelin/pattern.cc
  javelin/pattern_component.cc
  javelin/processor/all_up.cc
  javelin/processor/first_up.cc
  javelin/processor/gemini.cc
  javelin/processor/jeff_modifiers.cc
  javelin/processor/processor.cc
  javelin/processor/repeat.cc
  javelin/processor/switch.cc
  javelin/segment.cc
  javelin/state.cc
  javelin/steno_key_code.cc
  javelin/steno_key_code_buffer.cc
  javelin/steno_key_code_buffer_functions.cc
  javelin/steno_key_code_emitter.cc
  javelin/steno_key_state.cc
  javelin/string_util.cc
  javelin/utf8_pointer.cc
  javelin/windows_alt_unicode_data.cc
  javelin/word_list.cc
  javelin/word_list_data.cc
)

# Add the local directory so tinyusb can find tusb_config.h
target_include_directories(${PROJECT} PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)

family_configure_device_example(${PROJECT})
