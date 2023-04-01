# rp2040 bindings for javelin-steno.

## Build Instructions

1. Install [pico-sdk](https://github.com/raspberrypi/pico-sdk) and ensure that
   you can build the examples.

2. Clone [javelin-steno](https://github.com/jthlim/javelin-steno) repository.

3. Clone this repository.

4. From within this repository, link the javelin-steno repository:
   `ln -s <path-to-javelin-steno> javelin`

5. Standard CMake, with '-D BOARD=xxx'

- `mkdir build`
- `cd build`
- `cmake .. -D BOARD=uni_v4`
- `make`

You should now have a uf2 file that can be copied to the device.

# Contributions

Note that contributions are not currently being accepted until I get around
to setting up a Contributor License Agreement.

# Terms

This code is distributed under [PolyForm Noncommercial license](LICENSE.txt).
For commercial use, please [contact](mailto:jeff@lim.au) me.

# Troubleshooting

When building, if you receive a message like:

```
error: uinitialized const member in 'const char []'
```

This means that you're running an older version of the toolchain that has a
compiler bug. Update to a newer version at
https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads
