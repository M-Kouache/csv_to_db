#ifndef WORKER_THREADS_H
#define WORKER_THREADS_H

#include "benchmark.h"
#include "parsers.h"
#include <sys/_pthread/_pthread_cond_t.h>
#include <sys/_pthread/_pthread_mutex_t.h>

#define QUEUE_SIZE 10

typedef struct {
  ProcessedLocation *locations;
  int count;
} Batch;

// Thread-safe queue
typedef struct {
  Batch *batches[QUEUE_SIZE];
  int front;
  int rear;
  int count;
  pthread_mutex_t mutex;
  pthread_cond_t not_full;
  pthread_cond_t not_empty;
  bool done;
} BatchQueue;

// Worker thread context
typedef struct {
  int id;
  BatchQueue *queue;
  const char *conninfo;
  Benchmark *stats;
  pthread_mutex_t *stats_mutex;
} WorkerContext;

// Function prototypes
void queue_init(BatchQueue *queue);
void *worker_thread(void *arg);

#endif
