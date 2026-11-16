// the kernel's settings for this program. anything not set here takes its
// default from .freertos/include/FreeRTOS.h
#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#include "board.h"

#define configCPU_CLOCK_HZ CPU_HZ
#define configTICK_RATE_HZ 1000
#define configTICK_TYPE_WIDTH_IN_BITS TICK_TYPE_WIDTH_32_BITS
#define configUSE_PREEMPTION 1
#define configUSE_TIME_SLICING 1
#define configUSE_PORT_OPTIMISED_TASK_SELECTION 0
#define configMAX_PRIORITIES 5
#define configMINIMAL_STACK_SIZE 128
#define configMAX_TASK_NAME_LEN 12

#define configSUPPORT_DYNAMIC_ALLOCATION 1
#define configSUPPORT_STATIC_ALLOCATION 0
#define configTOTAL_HEAP_SIZE (16U * 1024U)

#define configUSE_IDLE_HOOK 0
#define configUSE_TICK_HOOK 1
#define configUSE_TIMERS 0
#define configCHECK_FOR_STACK_OVERFLOW 2
#define configUSE_MALLOC_FAILED_HOOK 1

// the Cortex-M33 port: FPU on, no MPU, no TrustZone
#define configENABLE_FPU 1
#define configENABLE_MPU 0
#define configENABLE_TRUSTZONE 0
#define configRUN_FREERTOS_SECURE_ONLY 0

// the H5 has 4 priority bits, in the top of each 8 bit field. interrupts
// at 5 to 15 may call the FromISR functions, 0 to 4 run past the kernel.
// the port puts PendSV and SysTick at 15 itself
#define configPRIO_BITS __NVIC_PRIO_BITS
#define configMAX_SYSCALL_INTERRUPT_PRIORITY (5U << (8U - configPRIO_BITS))

#define configASSERT(x)                                                        \
  do {                                                                         \
    if (!(x))                                                                  \
      fail();                                                                  \
  } while (0)

#define INCLUDE_vTaskDelay 1
#define INCLUDE_xTaskDelayUntil 1
#define INCLUDE_uxTaskGetStackHighWaterMark 1

#endif
