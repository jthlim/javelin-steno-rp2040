#include "javelin/thread.h"

#include <pico/multicore.h>

void RunParallel(void (*func1)(void *context), void *context1,
                 void (*func2)(void *context), void *context2) {

  multicore_fifo_push_blocking((uint32_t)func1);
  multicore_fifo_push_blocking((uint32_t)context1);

  (*func2)(context2);

  multicore_fifo_pop_blocking();
}

static void MultiCoreEntryPoint() {
  while (1) {
    void (*func)(void *) = (void (*)(void *))multicore_fifo_pop_blocking();
    void *context = (void *)multicore_fifo_pop_blocking();

    (*func)(context);

    multicore_fifo_push_blocking(0);
  }
}

void InitMulticore() {
  multicore_reset_core1();
  multicore_launch_core1(MultiCoreEntryPoint);
}