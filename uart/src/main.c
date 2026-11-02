// a tiny shell on the virtual com port (`just serial`): printf goes out over
// usart2 through common/syscalls.c, typed lines come back as commands for the
// led
#include <stdio.h>
#include <string.h>

#include "board.h"
#include "usart2.h"

// reads one line with echo and backspace
static void read_line(char *line, size_t cap) {
  size_t len = 0;
  for (;;) {
    char c = usart2_getc();
    if (c == '\r' || c == '\n') {
      printf("\n");
      line[len] = '\0';
      return;
    }
    if (c == '\b' || c == 0x7F) {
      if (len) {
        len--;
        printf("\b \b");
      }
    } else if (c >= ' ' && len < cap - 1U) {
      line[len++] = c;
      usart2_putc(c);
    }
  }
}

static void run(const char *cmd) {
  if (!strcmp(cmd, "on"))
    gpio_write(LD2, 1);
  else if (!strcmp(cmd, "off"))
    gpio_write(LD2, 0);
  else if (!strcmp(cmd, "toggle"))
    gpio_toggle(LD2);
  else if (*cmd)
    printf("commands: on off toggle\n");
}

int main(void) {
  gpio_output(LD2, 0);
  usart2_init(115200);
  // NOTE: unbuffered, or text without a newline sits in stdio's buffer
  setvbuf(stdout, NULL, _IONBF, 0);
  printf("nucleo-h533re, core at %lu MHz\n",
         (unsigned long)(CPU_HZ / 1000000U));

  char line[32];
  for (;;) {
    printf("> ");
    read_line(line, sizeof line);
    run(line);
  }
}
