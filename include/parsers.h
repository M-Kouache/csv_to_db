#ifndef PARSERS_H
#define PARSERS_H

#include <stdbool.h>
#include <stddef.h>


typedef struct {
  char change;
  char country_code[3];
  char location_code[4];
  char name[100];
  char name_wo_diacritics[100];
  char subdivision[100];
  char status[10];
  char function_code[9];
  char date[5];
  char iata[4];
  char coordinates[50];
  char remarks[200];
} LocationData;

// Structure to hold processed location data
typedef struct {
  char unlocode[6]; // country_code + location_code
  char name[100];
  char country_code[3];
  double latitude;
  double longitude;
  bool is_airport;
  bool is_port;
  bool is_train_station;
} ProcessedLocation;


void parse_line(char *line, LocationData *data);
void process_location_data(LocationData *raw, ProcessedLocation *processed);
bool parse_coordinates(const char *coord_str, double *lat, double *lon);
void parse_function_code(const char *function_code, bool *is_port,
                         bool *is_airport, bool *is_train);
char *escape_csv_field(const char *field, char *buffer, size_t buffer_size);

#endif
