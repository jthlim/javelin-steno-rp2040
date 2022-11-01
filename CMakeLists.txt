cmake_minimum_required(VERSION 3.12)

set(NAME javelin-steno-rp2040)

include(pico_sdk_import.cmake)

project(${NAME} C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

set(CMAKE_C_FLAGS "-O3 -mcpu=cortex-m0plus -mthumb -fomit-frame-pointer")
set(CMAKE_CXX_FLAGS "-O3 -mcpu=cortex-m0plus -mthumb -fomit-frame-pointer")

pico_sdk_init()

add_definitions(-DJAVELIN_THREADS)

add_executable(${NAME})

target_sources(${NAME} PUBLIC
  main.cc
  usb_descriptors.cc
  flash.cc
  key_state.cc
  console_buffer.cc
  hid_keyboard_report_builder.cc
  hid_report_buffer.cc
  pico_bindings.cc
  pico_thread.cc
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
  javelin/flash.cc
  javelin/key_press_parser.cc
  javelin/list.cc
  javelin/macos_us_unicode_data.cc
  javelin/orthography.cc
  javelin/pattern.cc
  javelin/pattern_component.cc
  javelin/processor/all_up.cc
  javelin/processor/first_up.cc
  javelin/processor/gemini.cc
  javelin/processor/jeff_modifiers.cc
  javelin/processor/plover_hid.cc
  javelin/processor/processor.cc
  javelin/processor/processor_list.cc
  javelin/processor/repeat.cc
  javelin/processor/switch.cc
  javelin/segment.cc
  javelin/state.cc
  javelin/steno_key_code.cc
  javelin/steno_key_code_buffer.cc
  javelin/steno_key_code_buffer_functions.cc
  javelin/steno_key_code_emitter.cc
  javelin/steno_key_state.cc
  javelin/str.cc
  javelin/utf8_pointer.cc
  javelin/windows_alt_unicode_data.cc
  javelin/word_list.cc
  javelin/word_list_data.cc
)

# Add the local directory so tinyusb can find tusb_config.h
target_include_directories(${NAME} PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(${NAME}
  hardware_clocks
  hardware_gpio
  pico_multicore
  pico_stdlib
  tinyusb_board
  tinyusb_device
)

# create map/bin/hex file etc.
pico_add_extra_outputs(${NAME})

