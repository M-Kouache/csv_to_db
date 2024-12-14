#ifndef BENCHMARK_H
#define BENCHMARK_H

typedef struct {
  double start_time;
  double parse_time;
  double db_time;
  int records_processed;
} Benchmark;

double get_time();

#endif
