/* See COPYRIGHT for copyright information. */

#ifndef _PRINTF_H_
#define _PRINTF_H_

void warn (const char *, ...);
void panic (const char *, ...) __attribute__ ((noreturn));
int printf (const char *, ...)__attribute__ ((format (printf, 1, 2)));

#define assert(x)                                                       \
do {                                                                    \
  if (! (x))                                                            \
    panic ("%s:%d: assertion failed: %s", __FILE__, __LINE__, #x);      \
} while (0)

#endif
