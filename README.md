# rp2040 bindings for javelin-steno.

## Build Instructions

1. Install [pico-sdk](https://github.com/raspberrypi/pico-sdk) and ensure that
   you can build the examples.

2. Clone [javelin-steno](https://github.com/jthlim/javelin-steno) repository.

3. Clone this repository.

4. From within this repository, link the javelin-steno repository:
`ln -s <path-to-javelin-steno>/javelin`

5. Standard CMake, with '-D BOARD=xxx'
  * `mkdir build`
  * `cd build`
  * `cmake .. -D BOARD=uni_v4`
  * `make`

You should now have a uf2 file that can be copied to the device.
