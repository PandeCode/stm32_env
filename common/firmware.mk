# shared by every c program. a program's Makefile lists its sources relative
# to its folder and includes this. run it from the repo root,
# `make -f tft/Makefile MODE=Release`, so paths in errors start at the root.
# the elf lands in <app>/build/<MODE>/<app>.elf
rel = $(patsubst $(CURDIR)/%,%,$(patsubst $(CURDIR),.,$(abspath $1)))

APPDIR := $(call rel,$(dir $(firstword $(MAKEFILE_LIST))))
COMMON := $(call rel,$(dir $(lastword $(MAKEFILE_LIST))))
ROOT := $(call rel,$(COMMON)/..)
APP := $(notdir $(abspath $(APPDIR)))
MODE ?= Debug
OUT := $(APPDIR)/build/$(MODE)
# st's startup (vector table, reset handler) and SystemInit, from the device
# package nix develop links in. a program with its own startup sets STARTUP
# empty
ST := $(ROOT)/.cmsis/templates
STARTUP ?= $(ST)/gcc/startup_stm32h533xx.s $(ST)/system_stm32h5xx.c
LDSCRIPT := $(ST)/gcc/linker/STM32H533xx_FLASH.ld
SRC := $(call rel,$(addprefix $(APPDIR)/,$(SRCS))) $(STARTUP)

CC := arm-none-eabi-gcc
MCU := -mcpu=cortex-m33 -mthumb -mfpu=fpv5-sp-d16 -mfloat-abi=hard
OPT_Debug := -Og -g3
OPT_Release := -Os -g
# nix develop links st's device and arm's core headers into .cmsis
CFLAGS := $(MCU) -std=gnu23 $(OPT_$(MODE)) -Wall -Wextra \
	-ffunction-sections -fdata-sections -DSTM32H533xx \
	-I$(COMMON) -I$(ROOT)/lib -I$(ROOT)/.cmsis/device -I$(ROOT)/.cmsis/core
LDFLAGS := $(MCU) -nostartfiles --specs=nano.specs --specs=nosys.specs \
	-Wl,--gc-sections -T$(LDSCRIPT) \
	-Wl,-Map=$(OUT)/$(APP).map -Wl,--print-memory-usage

# NOTE: st's startup calls __libc_init_array, which wants _init and _fini
# from crti/crtn. the rest of gcc's startfiles is crt0, a second startup that
# drags in exit and stdio
CRTI := $(if $(STARTUP),$(shell $(CC) $(MCU) -print-file-name=crti.o))
CRTN := $(if $(STARTUP),$(shell $(CC) $(MCU) -print-file-name=crtn.o))

# NOTE: objects are named after the file alone, so two sources can't share a
# name. it keeps ../ out of the object paths
OBJS := $(addprefix $(OUT)/obj/,$(notdir $(addsuffix .o,$(basename $(SRC)))))
vpath %.c $(sort $(dir $(SRC)))
vpath %.S $(sort $(dir $(SRC)))
vpath %.s $(sort $(dir $(SRC)))

.PHONY: all FORCE
all: $(OUT)/$(APP).elf $(ROOT)/build/compile_commands.json

$(OUT)/$(APP).elf: $(OBJS) $(LDSCRIPT)
	$(CC) $(LDFLAGS) -o $@ $(CRTI) $(OBJS) $(CRTN)

$(OUT)/obj/%.o: %.c $(APPDIR)/Makefile $(COMMON)/firmware.mk | $(OUT)/obj
	$(CC) $(CFLAGS) -MMD -MP -c -o $@ $<

$(OUT)/obj/%.o: %.s $(APPDIR)/Makefile $(COMMON)/firmware.mk | $(OUT)/obj
	$(CC) $(MCU) -g -c -o $@ $<

# .S goes through the c preprocessor first, so #define and #include work
$(OUT)/obj/%.o: %.S $(APPDIR)/Makefile $(COMMON)/firmware.mk | $(OUT)/obj
	$(CC) $(CFLAGS) -MMD -MP -c -o $@ $<

$(OUT)/obj:
	mkdir -p $@

# clangd looks for build/compile_commands.json in every folder above a source.
# the -o matches the build rule, so asm-lsp rerunning a command puts its object
# where the build does. rewritten every run, only touched when it changed
$(APPDIR)/build/compile_commands.json: FORCE | $(OUT)/obj
	@{ echo '['; sep=; for s in $(SRC); do \
		o=$(OUT)/obj/$$(basename "$${s%.*}").o; \
		case $$s in *.s) f='$(MCU) -g';; *) f='$(CFLAGS)';; esac; \
		printf '%s{"directory": "%s", "file": "%s", "command": "%s"}\n' \
			"$$sep" "$(CURDIR)" "$$s" "$(CC) $$f -c -o $$o $$s"; sep=,; \
	done; echo ']'; } >$@.new
	@cmp -s $@.new $@ && rm $@.new || mv $@.new $@

# every program's entries in one database at the root, for editors opened there
$(ROOT)/build/compile_commands.json: $(APPDIR)/build/compile_commands.json
	@mkdir -p $(@D)
	@awk 'BEGIN { print "[" } /^,?\{/ { sub(/^,/, ""); print (n++ ? "," : "") $$0 } \
		END { print "]" }' $(ROOT)/*/build/compile_commands.json >$@

-include $(OBJS:.o=.d)
