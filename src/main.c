#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parsers.c"
#include "db_query.c"
#include "worker_threads.c"
#include "benchmark.c"

#define MAX_LINE_LENGTH 1024
#define BATCH_SIZE 24000
#define NUM_WORKERS 3


int main(int argc, char *argv[]) {

  printf("Debug: Starting the program \n");

  if (argc != 2) {
    fprintf(stderr, "Usage: %s <path_to_csv_file>\n", argv[0]);
    return 1;
  }

  Benchmark stats = {.start_time = get_time(),
                     .parse_time = 0,
                     .db_time = 0,
                     .records_processed = 0};

  pthread_mutex_t stats_mutex = PTHREAD_MUTEX_INITIALIZER;

  BatchQueue queue;
  queue_init(&queue);

  pthread_t workers[NUM_WORKERS];

  WorkerContext contexts[NUM_WORKERS];
  const char *conninfo = "host=localhost port=5432 dbname=vessel_tracking "
                         "user=yourusername password=yourpassword";

  PGconn *conn = PQconnectdb(conninfo);

  if (PQstatus(conn) != CONNECTION_OK) {
    fprintf(stderr, "Worker Masin: Connection failed\n");
    return 0;
  }
  create_table(conn);
  PQfinish(conn);

  for (int i = 0; i < NUM_WORKERS; i++) {
    contexts[i].id = i;
    contexts[i].queue = &queue;
    contexts[i].conninfo = conninfo;
    contexts[i].stats = &stats;
    contexts[i].stats_mutex = &stats_mutex;
    pthread_create(&workers[i], NULL, worker_thread, &contexts[i]);
  }

  printf("Debug: Allocating memory for the batch size %d \n", BATCH_SIZE);

  double total_start_time = get_time();

  // Open input file
  printf("Debug: Opening file for proccessing \n");
  FILE *file = fopen(argv[1], "r");
  if (!file) {
    fprintf(stderr, "Could not open input file\n");
    return 1;
  }

  printf("Debug: Starting main processing loop\n");
  char line[MAX_LINE_LENGTH];

    /*// Skip header line*/
  if (fgets(line, MAX_LINE_LENGTH, file) == NULL) {
    fprintf(stderr, "Failed to read header line\n");
    fclose(file);
    return 1;
  }

  // create memory to hold Batch values
  Batch *current_batch = malloc(sizeof(Batch));
  current_batch->locations = malloc(BATCH_SIZE * sizeof(ProcessedLocation));
  current_batch->count = 0;

  printf("Debug: Process file line by line\n");
  while (fgets(line, MAX_LINE_LENGTH, file)) {
    double parse_start = get_time();

    LocationData raw_data;
    ProcessedLocation processed_data;

    line[strcspn(line, "\n")] = 0;

    // Parse and process data
    parse_line(line, &raw_data);
    process_location_data(&raw_data, &processed_data);

    stats.parse_time = get_time() - parse_start;

    // Add to batch
    memcpy(&current_batch->locations[current_batch->count], &processed_data,
           sizeof(ProcessedLocation));
    current_batch->count++;

    // If batch is full, insert and reset
    if (current_batch->count == BATCH_SIZE) {
      queue_push(&queue, current_batch);
      current_batch = malloc(sizeof(Batch));
      current_batch->locations = malloc(BATCH_SIZE * sizeof(ProcessedLocation));
      current_batch->count = 0;
    }
  }

  // Push final batch if not empty
  if (current_batch->count > 0) {
    queue_push(&queue, current_batch);
  } else {
    free(current_batch->locations);
    free(current_batch);
  }

  // Singal workers to finish
  pthread_mutex_lock(&queue.mutex);
  queue.done = true;
  pthread_cond_broadcast(&queue.not_empty);
  pthread_mutex_unlock(&queue.mutex);

  for (int i = 0; i < NUM_WORKERS; i++) {
    pthread_join(workers[i], NULL);
  }

  double total_time = get_time() - stats.start_time;

  /*
   * the Benchmark might need further refuctoring,
   * some of the stats like records_processed might be wrong!!
   * */
  printf("\nBenchmark Results:\n");
  printf("Total records processed: %d\n", stats.records_processed);
  printf("Total time: %.2f seconds\n", total_time);
  printf("File parsing time: %.8f seconds (%.1f%%)\n", stats.parse_time,
         (stats.parse_time / total_time) * 100);
  printf("Database time: %.2f seconds (%.1f%%)\n", stats.db_time,
         (stats.db_time / total_time) * 100);
  printf("Other operations: %.2f seconds (%.1f%%)\n",
         total_time - (stats.parse_time + stats.db_time),
         ((total_time - (stats.parse_time + stats.db_time)) / total_time) *
             100);
  printf("Average time per record: %.6f seconds\n",
         total_time / stats.records_processed);
  printf("Records per second: %.1f\n", stats.records_processed / total_time);

  printf("Debug: Cleanup \n");

  pthread_mutex_destroy(&queue.mutex);
  pthread_cond_destroy(&queue.not_full);
  pthread_cond_destroy(&queue.not_empty);
  pthread_mutex_destroy(&stats_mutex);
  fclose(file);

  return 0;
}




