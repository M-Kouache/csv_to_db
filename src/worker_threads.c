#include "worker_threads.h"
#include <pthread.h>
#include <stdbool.h>
#include <string.h>

void queue_init(BatchQueue *queue) {
  queue->front = 0;
  queue->rear = 0;
  queue->count = 0;
  queue->done = false;
  pthread_mutex_init(&queue->mutex, NULL);
  pthread_cond_init(&queue->not_full, NULL);
  pthread_cond_init(&queue->not_empty, NULL);
  memset(queue->batches, 0, sizeof(queue->batches));
}

void queue_push(BatchQueue *queue, Batch *batch) {

  /*
   * prevent race condition or dead lock by locking the queue when it's in use
   * by a consumer thread, or a pushing producer (main thread), The
   * pthread_mutex_lock() function locks mutex.  If the mutex is already locked,
   * the calling thread will block until the mutex becomes available.
   * */
  pthread_mutex_lock(&queue->mutex);

  /*
 if the queue is full wait for the consumer
 to pop a task from the queue
 the pthread_cond_wait() function atomically blocks the current thread waiting
 on the condition variable specified by cond (in our example cond is not_full
 attr), and releases the mutex specified by mutex.  The waiting thread
 unblocks only after another thread calls pthread_cond_signal(3), or
 pthread_cond_broadcast(3) with the same condition variable, and the current
 thread reacquires the lock on mutex.
 Along def i know don't judge i just wanted to it make clear :)
 */
  while (queue->count == QUEUE_SIZE) {
    pthread_cond_wait(&queue->not_full, &queue->mutex);
  }

  queue->batches[queue->rear] = batch;
  queue->rear = (queue->rear + 1) % QUEUE_SIZE;
  queue->count++;

  pthread_cond_signal(&queue->not_empty);
  pthread_mutex_unlock(&queue->mutex);
}

Batch *queue_pop(BatchQueue *queue) {
  pthread_mutex_lock(&queue->mutex);
  while (queue->count == 0 && !queue->done) {
    pthread_cond_wait(&queue->not_empty, &queue->mutex);
  }

  if (queue->count == 0 && queue->done) {
    pthread_mutex_unlock(&queue->mutex);
    return NULL;
  }

  Batch *batch = queue->batches[queue->front];
  queue->front = (queue->front + 1) % QUEUE_SIZE;
  queue->count--;

  pthread_cond_signal(&queue->not_full);
  pthread_mutex_unlock(&queue->mutex);
  return batch;
}

// Worker thread function
void *worker_thread(void *arg) {
  WorkerContext *ctx = (WorkerContext *)arg;
  PGconn *conn = PQconnectdb(ctx->conninfo);

  if (PQstatus(conn) != CONNECTION_OK) {
    fprintf(stderr, "Worker %d: Connection failed\n", ctx->id);
    return NULL;
  }

  while (true) {
    Batch *batch = queue_pop(ctx->queue);
    if (batch == NULL)
      break; // Queue is done

    double start_time = get_time();
    bool success = batch_insert_locations(conn, batch->locations, batch->count);
    double end_time = get_time();

    if (success) {
      pthread_mutex_lock(ctx->stats_mutex);
      ctx->stats->db_time += (end_time - start_time);
      ctx->stats->records_processed += batch->count;
      pthread_mutex_unlock(ctx->stats_mutex);
    }

    free(batch->locations);
    free(batch);
  }

  PQfinish(conn);
  return NULL;
}
