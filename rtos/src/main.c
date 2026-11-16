// FreeRTOS on the board, three tasks:
// - tick, priority 3: wakes on every 1 ms tick and measures how long after
//   the tick it got to run. PC5 (CN10-6) goes high in the tick interrupt and
//   low when the task runs, so on a logic analyzer the pulse is the delay
// - report, priority 2: prints what tick saw once a second, the printf is
//   slow enough that tick has to preempt it
// - blink, priority 1: LD2 at 1 Hz
#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"

#include "board.h"
#include "usart2.h"

#define TIMING GPIOC, 5

static TaskHandle_t tick_task, report_task, blink_task;

// written by tick, read and cleared by report
static volatile uint32_t late_max, late_sum, wakes;

static uint32_t cycles_to_us(uint32_t cycles) {
  return cycles / (CPU_HZ / 1000000U);
}

static void tick(void *arg) {
  (void)arg;
  TickType_t wake = xTaskGetTickCount();

  for (;;) {
    xTaskDelayUntil(&wake, 1);
    // systick counts down from LOAD and reloads on the tick, so LOAD - VAL
    // is how many cycles ago the tick fired: the interrupt, the scheduler
    // and the switch to this task
    uint32_t late = SysTick->LOAD - SysTick->VAL;
    gpio_write(TIMING, 0);
    if (late > late_max)
      late_max = late;
    late_sum += late;
    wakes++;
  }
}

static void report(void *arg) {
  (void)arg;
  TickType_t wake = xTaskGetTickCount();

  for (;;) {
    xTaskDelayUntil(&wake, pdMS_TO_TICKS(1000));
    taskENTER_CRITICAL();
    uint32_t max = late_max, sum = late_sum, n = wakes;
    late_max = late_sum = wakes = 0;
    taskEXIT_CRITICAL();

    printf("%lu wakes, late %lu cycles avg %lu max (%lu us)\n",
           (unsigned long)n, (unsigned long)(n ? sum / n : 0),
           (unsigned long)max, (unsigned long)cycles_to_us(max));
    printf("stack left in words: tick %lu report %lu blink %lu, heap %lu\n",
           (unsigned long)uxTaskGetStackHighWaterMark(tick_task),
           (unsigned long)uxTaskGetStackHighWaterMark(report_task),
           (unsigned long)uxTaskGetStackHighWaterMark(blink_task),
           (unsigned long)xPortGetFreeHeapSize());
  }
}

static void blink(void *arg) {
  (void)arg;
  for (;;) {
    gpio_toggle(LD2);
    vTaskDelay(pdMS_TO_TICKS(500));
  }
}

// configUSE_TICK_HOOK calls this in the tick interrupt
void vApplicationTickHook(void) { gpio_write(TIMING, 1); }

// configCHECK_FOR_STACK_OVERFLOW and configUSE_MALLOC_FAILED_HOOK call these
void vApplicationStackOverflowHook(TaskHandle_t task, char *name) {
  (void)task;
  (void)name;
  fail();
}

void vApplicationMallocFailedHook(void) { fail(); }

int main(void) {
  gpio_output(LD2, 0);
  gpio_output(TIMING, 0);
  usart2_init(115200);
  setvbuf(stdout, NULL, _IONBF, 0);
  printf("freertos %s, core at %lu MHz\n", tskKERNEL_VERSION_NUMBER,
         (unsigned long)(CPU_HZ / 1000000U));

  // printf wants a bigger stack than the others
  if (xTaskCreate(tick, "tick", configMINIMAL_STACK_SIZE, NULL, 3,
                  &tick_task) != pdPASS ||
      xTaskCreate(report, "report", 4U * configMINIMAL_STACK_SIZE, NULL, 2,
                  &report_task) != pdPASS ||
      xTaskCreate(blink, "blink", configMINIMAL_STACK_SIZE, NULL, 1,
                  &blink_task) != pdPASS)
    fail();

  vTaskStartScheduler();
  // only gets here when the heap can't hold the idle task
  fail();
}
