#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

int getint() {
  int tmp;
  scanf("%d", &tmp);
  return tmp;
}
void putint(int x) { printf("%d", x); }
void putch(int ch) { putchar(ch); }
void putf(const char* format, ...) {
  va_list args;
  const char* args1;
  va_start(args, format);
  args1 = va_arg(args, const char*);
  va_end(args);
  printf(format, args1);
}
void putarray(int num, int a[]) {
  printf("%d", num);
  for (int i = 0; i < num; i++) {
    printf(" %d", a[i]);
  }
}
int getarray(int x[]) {
  int len = x[0];
  for (int i = 0; i < len; i++) {
    scanf("%d", &x[i]);
  }
  return len;
}
int setup_exit_fun = 0;
int counter = 1;
static struct timeval st;
static struct timeval ed;
static long second_total;
static long us_total;
void showtime(void) {
  long hour = second_total / 3600;
  long minite = second_total / 60;
  long sencond = second_total % 60;
  fprintf(stderr, "TOTAL:%dH-%dM-%dS-%dus\n", hour, minite, sencond, us_total);
}
void starttime() {
  gettimeofday(&st, NULL);
  if (!setup_exit_fun) {
    atexit(showtime);
    setup_exit_fun = 1;
  }
}
void stoptime() {
  gettimeofday(&ed, NULL);
  long hour = (ed.tv_sec - st.tv_sec) / 3600;
  long minite = (ed.tv_sec - st.tv_sec) / 60;
  long sencond = (ed.tv_sec - st.tv_sec) % 60;
  second_total += ed.tv_sec - st.tv_sec;
  us_total += ed.tv_usec - st.tv_usec;
  fprintf(stderr, "Timer#%03d@0010-0012:%dH-%dM-%dS-%dus\n", counter, hour,
          minite, sencond, ed.tv_usec - st.tv_usec);
}
