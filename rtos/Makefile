# the FreeRTOS kernel comes from .freertos, which nix develop links in, with
# the Cortex-M33 port for a chip running without TrustZone
RTOS := ../.freertos
PORT := $(RTOS)/portable/GCC/ARM_CM33_NTZ/non_secure

SRCS := src/main.c ../common/board.c ../common/usart2.c ../common/syscalls.c \
	$(RTOS)/tasks.c $(RTOS)/list.c $(RTOS)/queue.c \
	$(RTOS)/portable/MemMang/heap_4.c $(PORT)/port.c $(PORT)/portasm.c
INCS := src $(RTOS)/include $(PORT)

include $(dir $(lastword $(MAKEFILE_LIST)))../common/firmware.mk
