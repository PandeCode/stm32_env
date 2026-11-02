// newlib's stdio on usart2: printf writes to it, scanf and getchar read from
// it. the rest are the stubs newlib needs for a single console
#include <sys/stat.h>

#include "usart2.h"

int _write(int fd, const char *buf, int len) {
  (void)fd;
  for (int i = 0; i < len; i++) {
    // terminals want \r\n
    if (buf[i] == '\n')
      usart2_putc('\r');
    usart2_putc(buf[i]);
  }
  return len;
}

int _read(int fd, char *buf, int len) {
  (void)fd;
  if (len < 1)
    return 0;
  // one byte at a time, so a line reads as soon as it is typed
  buf[0] = usart2_getc();
  return 1;
}

int _close(int fd) {
  (void)fd;
  return -1;
}

int _lseek(int fd, int off, int whence) {
  (void)fd;
  (void)off;
  (void)whence;
  return 0;
}

int _fstat(int fd, struct stat *st) {
  (void)fd;
  st->st_mode = S_IFCHR;
  return 0;
}

int _isatty(int fd) {
  (void)fd;
  return 1;
}
