# every program is a folder with a Makefile, a build.zig, or a CMakePresets.json
# when CubeMX made it. build and clean take all of them by default, the rest
# blink. pyocd reads its pack and target from pyocd.yaml, so recipes run from
# here

app := "blink"
mode := "Debug"
programs := `for d in */; do d=${d%/}; for f in Makefile build.zig CMakePresets.json; do if [ -e "$d/$f" ]; then echo "$d"; break; fi; done; done`

_default:
    @just --list

# build one program or all of them, the elf lands in <app>/build/<mode>/<app>.elf
build app="all" mode=mode:
    #!/usr/bin/env bash
    set -euo pipefail
    for app in {{ if app == "all" { replace(programs, "\n", " ") } else { app } }}; do
        if [ -e "$app/build.zig" ]; then
            zig build --build-file "$app/build.zig" -p "$app/build/{{ mode }}" \
                -Doptimize={{ if mode == "Release" { "ReleaseSmall" } else { "Debug" } }}
        elif [ -e "$app/Makefile" ]; then
            make -f "$app/Makefile" MODE={{ mode }}
        elif [ -e "$app/CMakePresets.json" ]; then
            cmake --preset {{ mode }} -S "$app"
            cmake --build "$app/build/{{ mode }}"
        else
            echo "no program $app" >&2
            exit 1
        fi
    done

# build and flash, then the board runs it
flash app=app mode=mode: (build app mode)
    pyocd flash {{ app }}/build/{{ mode }}/{{ app }}.elf

# flash and step through it in gf2, stops at main
debug app=app mode=mode: (build app mode)
    #!/usr/bin/env bash
    set -euo pipefail
    mkdir -p build
    pyocd gdbserver --persist >build/gdbserver.log 2>&1 &
    trap 'kill $!' EXIT
    until (exec 3<>/dev/tcp/127.0.0.1/3333) 2>/dev/null; do
        kill -0 $! 2>/dev/null || { cat build/gdbserver.log >&2; exit 1; }
        sleep 0.2
    done
    # gf2 runs the shell's gdb, which has the python its watch window needs
    # pyocd's memory map has no peripherals, so gdb would refuse to read them
    gf2 {{ app }}/build/{{ mode }}/{{ app }}.elf -x gdb/svd.py \
        -ex 'set mem inaccessible-by-default off' \
        -ex 'target extended-remote :3333' -ex load -ex 'monitor reset halt' \
        -ex 'break main' -ex continue

# gdb server on :3333 for an editor or a another gdb
gdbserver:
    pyocd gdbserver

# flash and ram size of a build
size app=app mode=mode:
    arm-none-eabi-size {{ app }}/build/{{ mode }}/{{ app }}.elf

# reset the board
reset:
    pyocd reset

# erase the whole flash
erase:
    pyocd erase --chip

# what's plugged in: the st-link on usb, then what pyocd and stlink see
probe:
    -lsusb | grep 0483
    st-info --probe
    pyocd list

# the board's virtual com port
serial port="/dev/ttyACM0" baud="115200":
    tio -b {{ baud }} {{ port }}

# set up pins and clocks, generate the HAL and FreeRTOS code
cubemx:
    stm32cubemx

# the logic analyzer gui
logic:
    pulseview

# format the whole repo
fmt:
    treefmt

# drop a program's build folder, or every one and the root database
clean app="all":
    rm -rf {{ if app == "all" { "build */build */.zig-cache" } else { app + "/build " + app + "/.zig-cache" } }}
