cmake_minimum_required(VERSION 3.12)
cmake_policy(SET CMP0076 NEW)

set(NAME javelin-steno-rp2040)
set(JAVELIN_BOARD "" CACHE STRING "Target board, e.g. \"uni_v4\"")

if ("${JAVELIN_BOARD}" STREQUAL "")
  message(FATAL_ERROR, "Target board (e.g. 'uni_v4') must be specified")
endif()

add_definitions(
  -DJAVELIN_BOARD_CONFIG=\"config/${JAVELIN_BOARD}.h\"
  -DJAVELIN_PLATFORM_PICO_SDK=1
  -DJAVELIN_THREADS=1
  -DJAVELIN_CPU_CORTEX_M0=1
  -DJAVELIN_USE_CUSTOM_POP_COUNT=1
  -DNDEBUG=1
  -DPICO_FLASH_SIZE_BYTES=0x1000000
  -DPICO_FLASH_SPI_CLKDIV=10
  -DPICO_MALLOC_PANIC=0
  -DPICO_NO_FPGA_CHECK=1
  -DPICO_PRINTF_SUPPORT_FLOAT=0
  -DPICO_PRINTF_SUPPORT_PTRDIFF_T=0
)

include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(${NAME} C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

set(CMAKE_C_FLAGS "-O2 -g -mcpu=cortex-m0plus")
set(CMAKE_CXX_FLAGS "-O2 -g -mcpu=cortex-m0plus -fno-exceptions -fno-devirtualize-speculatively -save-temps")

pico_sdk_init()

add_executable(${NAME})

pico_generate_pio_header(${NAME} ${CMAKE_CURRENT_LIST_DIR}/rp2040_split.pio)
pico_generate_pio_header(${NAME} ${CMAKE_CURRENT_LIST_DIR}/rp2040_ws2812.pio)

target_sources(${NAME} PUBLIC
  main.cc
  auto_draw.cc
  console_report_buffer.cc
  hid_keyboard_report_builder.cc
  hid_report_buffer.cc
  libc_stubs.cc
  pico_bindings.cc
  pico_thread.cc
  plover_hid_report_buffer.cc
  rp2040_bootloader.cc
  rp2040_button_state.cc
  rp2040_clock.cc
  rp2040_console.cc
  rp2040_crc.cc
  rp2040_flash.cc
  rp2040_gpio.cc
  rp2040_orthography.cc
  rp2040_random.cc
  rp2040_serial_port.cc
  rp2040_split.cc
  rp2040_ws2812.cc
  split_hid_report_buffer.cc
  ssd1306.cc
  ssd1306_paper_tape.cc
  ssd1306_steno_layout.cc
  usb_descriptors.cc

  javelin/base64.cc
  javelin/bit.cc
  javelin/console.cc
  javelin/console_input_buffer.cc
  javelin/crc.cc
  javelin/dictionary/compact_map_dictionary.cc
  javelin/dictionary/compact_test_dictionary.cc
  javelin/dictionary/corrupted_dictionary.cc
  javelin/dictionary/debug_dictionary.cc
  javelin/dictionary/dictionary.cc
  javelin/dictionary/dictionary_definition.cc
  javelin/dictionary/dictionary_list.cc
  javelin/dictionary/emily_symbols_dictionary.cc
  javelin/dictionary/jeff_numbers_dictionary.cc
  javelin/dictionary/jeff_phrasing_dictionary.cc
  javelin/dictionary/jeff_phrasing_dictionary_data.cc
  javelin/dictionary/jeff_show_stroke_dictionary.cc
  javelin/dictionary/reverse_auto_suffix_dictionary.cc
  javelin/dictionary/reverse_map_dictionary.cc
  javelin/dictionary/reverse_prefix_dictionary.cc
  javelin/dictionary/unicode_dictionary.cc
  javelin/dictionary/user_dictionary.cc
  javelin/dictionary/wrapped_dictionary.cc
  javelin/engine_add_translation_mode.cc
  javelin/engine_binding.cc
  javelin/engine_normal_mode.cc
  javelin/engine.cc
  javelin/flash.cc
  javelin/font/monochrome/data/default.cc
  javelin/font/monochrome/data/small_digits.cc
  javelin/font/monochrome/data/medium_digits.cc
  javelin/font/monochrome/data/large_digits.cc
  javelin/font/monochrome/font.cc
  javelin/hal/ble.cc
  javelin/hal/bootloader.cc
  javelin/hal/connection.cc
  javelin/hal/display.cc
  javelin/hal/power.cc
  javelin/hal/rgb.cc
  javelin/hal/usb_status.cc
  javelin/key.cc
  javelin/key_code.cc
  javelin/keyboard_layout.cc
  javelin/key_press_parser.cc
  javelin/list.cc
  javelin/macos_us_unicode_data.cc
  javelin/orthography.cc
  javelin/pattern.cc
  javelin/pattern_component.cc
  javelin/pool_allocate.cc
  javelin/processor/all_up.cc
  javelin/processor/first_up.cc
  javelin/processor/gemini.cc
  javelin/processor/jeff_modifiers.cc
  javelin/processor/plover_hid.cc
  javelin/processor/procat.cc
  javelin/processor/processor.cc
  javelin/processor/processor_list.cc
  javelin/processor/repeat.cc
  javelin/processor/tx_bolt.cc
  javelin/random.cc
  javelin/script.cc
  javelin/script_byte_code.cc
  javelin/script_manager.cc
  javelin/segment.cc
  javelin/split/pair_console.cc
  javelin/split/split.cc
  javelin/split/split_serial_buffer.cc
  javelin/split/split_usb_status.cc
  javelin/state.cc
  javelin/steno_key_code.cc
  javelin/steno_key_code_buffer.cc
  javelin/steno_key_code_buffer_functions.cc
  javelin/steno_key_code_emitter.cc
  javelin/steno_key_state.cc
  javelin/str.cc
  javelin/stroke.cc
  javelin/stroke_history.cc
  javelin/stroke_list_parser.cc
  javelin/timer_manager.cc
  javelin/unicode.cc
  javelin/utf8_pointer.cc
  javelin/windows_alt_unicode_data.cc
  javelin/word_list.cc
  javelin/word_list_data.cc
  javelin/wpm_tracker.cc
  javelin/writer.cc
)

# Add the local directory so tinyusb can find tusb_config.h
target_include_directories(${NAME} PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(${NAME}
  hardware_clocks
  hardware_flash
  hardware_gpio
  hardware_i2c
  hardware_pio
  pico_multicore
  pico_printf
  pico_stdlib
  tinyusb_device
)

# create map/bin/hex file etc.
pico_add_extra_outputs(${NAME})

