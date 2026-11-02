# stm32 env

Firmware for a NUCLEO-H533RE (STM32H533RET6, Cortex-M33 with FPU, on-board
ST-LINK/V3EC). Register-level C on ST's device headers, drivers for
Adafruit's 1.8" TFT shield, and a Zig blink.

## Shell

    nix develop

Has stm32cubemx, arm-none-eabi-gcc and arm-none-eabi-gdb (gcc-arm-embedded),
make, clangd, asm-lsp, cmake and ninja (CubeMX only), just, pyocd, stlink,
gdb, gf2, tio, zig 0.17 with zls, sigrok-cli, pulseview and treefmt.

It links into the repo root:

- `.pyocd/`: the STM32H5 pack and the SVD. `pyocd.yaml` loads the pack and
  sets the target, so no `-t` and no `pyocd pack install`
- `.cmsis/device`: ST's device headers, `stm32h5xx.h`
- `.cmsis/core`: Arm's CMSIS 6 core headers (SysTick, SCB, NVIC)
- `.cmsis/templates`: ST's startup, `system_stm32h5xx.c` and linker scripts

## just

    just                        # list the recipes
    just build                  # every program in Debug
    just build uart Release     # one program
    just clean                  # every build folder, `just clean uart` for one
    just flash                  # build and flash blink, LD2 blinks at 1 Hz
    just flash tft Release
    just debug uart             # flash, then gf2 stopped at main
    just serial                 # tio on /dev/ttyACM0 at 115200
    just probe                  # lsusb, st-info --probe, pyocd list

`.project.gf` puts buttons in gf2's Commands tab: step, restart, reflash,
registers.

A program is a folder with a `Makefile`, a `build.zig`, or a
`CMakePresets.json` from CubeMX. The elf lands in
`<app>/build/<mode>/<app>.elf`.

## Layout

- `common/`: shared by the C programs
  - `firmware.mk`: flags and rules. Links ST's startup and linker script
    unless the program sets `STARTUP` empty, and writes
    `compile_commands.json` for clangd
  - `board.c`: SysTick delays, GPIO setup
  - `usart2.c`, `spi1.c`, `i2c1.c`, `adc1.c`: polling drivers
  - `syscalls.c`: `_write`/`_read` on USART2 for printf and scanf
- `lib/`: drivers with no MCU code, the header holds the code for the one .c
  file that defines `<NAME>_IMPLEMENTATION`. The caller passes the bus in
  - `st7735.h`: the TFT panel: init, rotation, filled rects
  - `seesaw.h`: Adafruit seesaw over I2C: GPIO, PWM, reset
- `gdb/svd.py`: `svd SPI1`, `svd SPI1 SR` in gdb print the registers with
  their fields, from the SVD. `just debug` loads it

A program's Makefile:

    SRCS := src/main.c ../common/board.c ../common/usart2.c

    include $(dir $(lastword $(MAKEFILE_LIST)))../common/firmware.mk

## Programs

- `blink`: the default. PA5 (LD2) toggles every 500 ms off SysTick, only
  `main.c` and ST's startup
- `asm-blink`: the same in Thumb-2 assembly with its own vector table, about
  170 bytes
- `zig-blink`: the same with the startup in Zig. `build.zig` runs ST's
  header through translate-c, and it has its own linker script since lld
  can't read ST's
- `uart`: a shell on the virtual com port, `on`, `off`, `toggle`
- `temp`: the chip's temperature and VDDA on the virtual com port, from
  the ADC and ST's factory calibration
- `tft`: Adafruit's 1.8" TFT shield v2 (802). The joystick moves a square,
  pressing it changes the color, A/B/C set the backlight

## TFT shield

The shield takes SPI from the 6-pin ICSP header, which the Nucleo doesn't
have. Wire ICSP SCK (pin 3) to D13, MOSI (4) to D11 and MISO (1) to D12.
MISO is only for the SD card.

| what           | shield       | nucleo        |
| -------------- | ------------ | ------------- |
| TFT SPI        | ICSP         | PA5, PA7 SPI1 |
| TFT DC         | D8           | PC7           |
| TFT CS         | D10          | PC9           |
| SD CS (high)   | D4           | PB5           |
| seesaw at 0x2E | SCL, SDA     | PB6, PB7 I2C1 |
| TFT reset      | seesaw pin 3 |               |
| backlight      | seesaw PWM 0 |               |

D13 is LD2 too, so it flickers while the screen draws. The SD slot isn't
used yet.

## Board

Upstream OpenOCD 0.12 has no STM32H5 support, so it's pyocd:

    pyocd flash blink/build/Debug/blink.elf
    pyocd gdbserver
    arm-none-eabi-gdb blink/build/Debug/blink.elf -ex 'target remote :3333'

The VCP is USART2 on PA2/PA3. Arduino D0/D1 is USART1 on PB15/PB14.

## CubeMX

Generate into its own folder next to `blink`. In Project Manager pick CMake
as the toolchain, GCC as the compiler (ST's default, starm-clang, isn't in
nixpkgs) and "copy only the necessary library files". Name the project after
its folder, `just` looks for `<folder>/build/<mode>/<folder>.elf`.

The `.ioc` is the source. Regenerating keeps only what sits between the
`USER CODE BEGIN` and `USER CODE END` lines, so keep `main.c` to a call into
a file of your own, like `Core/Src/app.c`. treefmt skips what CubeMX
writes and formats only those files.

## Sources

Pinned by hash in the flake, nothing copied in.

- `.cmsis/device`, `.cmsis/templates`:
  [STMicroelectronics/cmsis-device-h5](https://github.com/STMicroelectronics/cmsis-device-h5)
  v1.7.0
- `.cmsis/core`: [ARM-software/CMSIS_6](https://github.com/ARM-software/CMSIS_6)
  v6.4.0
- `.pyocd/`: the
  [STM32H5xx_DFP 2.3.1 pack](https://github.com/Open-CMSIS-Pack/STM32H5xx_DFP/releases/tag/v2.3.1),
  the SVD is `CMSIS/SVD/STM32H533.svd` from it

Where the register values come from is in `notes.md`.
