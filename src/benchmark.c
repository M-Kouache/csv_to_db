#include "benchmark.h"
#include <_time.h>



// Function to get current time in seconds
double get_time() {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts.tv_sec + ts.tv_nsec / 1e9;
}
