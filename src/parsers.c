#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "parsers.h"

// Parse a single CSV line into LocationData structure
void parse_line(char *line, LocationData *data) {
  // Initialize the structure to zero first
  memset(data, 0, sizeof(LocationData));

  // Skip empty lines or lines with just quotes
  if (line == NULL || strlen(line) <= 1) {
    return;
  }

  char *rest = line;
  char *token;

  // Change
  token = strsep(&rest, ",");
  if (token && strlen(token) > 0) {
    data->change = token[0];
  }

  // Country
  token = strsep(&rest, ",");
  if (token && strlen(token) > 0) {
    strncpy(data->country_code, token, 2);
    data->country_code[2] = '\0';
  }

  // Location
  token = strsep(&rest, ",");
  if (token && strlen(token) > 0) {
    strncpy(data->location_code, token, 3);
    data->location_code[3] = '\0';
  }

  // Name
  token = strsep(&rest, ",");
  if (token && strlen(token) > 0) {
    if (strlen(token) > 255) {
      printf("Warning: Long name found (%zu chars): %s \n", strlen(token),
             token);
    }
    strncpy(data->name, token, 99);
    data->name[99] = '\0';
  }

  // Name without diacritics
  token = strsep(&rest, ",");
  if (token && strlen(token) > 0) {
    strncpy(data->name_wo_diacritics, token, 99);
    data->name_wo_diacritics[99] = '\0';
  }

  // Subdivision
  token = strsep(&rest, ",");
  if (token && strlen(token) > 0) {
    strncpy(data->subdivision, token, 99);
    data->subdivision[99] = '\0';
  }

  // Status
  token = strsep(&rest, ",");
  if (token && strlen(token) > 0) {
    strncpy(data->status, token, 9);
    data->status[9] = '\0';
  }

  // Function
  token = strsep(&rest, ",");
  if (token && strlen(token) > 0) {
    strncpy(data->function_code, token, 8);
    data->function_code[8] = '\0';
  }

  // Date
  token = strsep(&rest, ",");
  if (token && strlen(token) > 0) {
    strncpy(data->date, token, 4);
    data->date[4] = '\0';
  }

  // IATA
  token = strsep(&rest, ",");
  if (token && strlen(token) > 0) {
    strncpy(data->iata, token, 3);
    data->iata[3] = '\0';
  }

  // Coordinates
  token = strsep(&rest, ",");
  if (token && strlen(token) > 0) {
    strncpy(data->coordinates, token, 49);
    data->coordinates[49] = '\0';
  }

  // Remarks - careful with the trailing quote
  token = strsep(&rest, ",");
  if (token) {
    // Remove quotes if present
    size_t len = strlen(token);
    if (len > 0) {
      if (token[len - 1] == '"')
        token[len - 1] = '\0';
      if (token[0] == '"')
        token++;
      strncpy(data->remarks, token, 199);
      data->remarks[199] = '\0';
    }
  }
}

// Parse coordinate string into latitude and longitude
bool parse_coordinates(const char *coord_str, double *lat, double *lon) {
  if (!coord_str || strlen(coord_str) == 0) {
    *lat = 0;
    *lon = 0;
    return false;
  }

  // Expected format: "DDMM[N/S] DDDMM[E/W]"
  int lat_deg, lat_min, lon_deg, lon_min;
  char lat_dir, lon_dir;

  if (sscanf(coord_str, "%2d%2d%c %3d%2d%c", &lat_deg, &lat_min, &lat_dir,
             &lon_deg, &lon_min, &lon_dir) != 6) {
    *lat = 0;
    *lon = 0;
    return false;
  }

  *lat = lat_deg + lat_min / 60.0;
  *lon = lon_deg + lon_min / 60.0;

  if (lat_dir == 'S')
    *lat = -*lat;
  if (lon_dir == 'W')
    *lon = -*lon;

  return true;
}

// Parse function code to determine location type
void parse_function_code(const char *function_code, bool *is_port,
                         bool *is_airport, bool *is_train) {
  *is_port = false;
  *is_airport = false;
  *is_train = false;

  if (!function_code || strlen(function_code) < 8)
    return;

  for (int i = 0; i < 8; i++) {
    switch (function_code[i]) {
    case '1':
      *is_port = true;
      break;
    case '2':
      *is_train = true;
      break;
    case '4':
      *is_airport = true;
      break;
    }
  }
}

// Process raw location data into final format
void process_location_data(LocationData *raw, ProcessedLocation *processed) {
  // Create UNLOCODE (country + location)
  snprintf(processed->unlocode, 6, "%s%s", raw->country_code,
           raw->location_code);

  // Copy name and country code
  strncpy(processed->name, raw->name, 99);
  processed->name[99] = '\0';
  strncpy(processed->country_code, raw->country_code, 2);
  processed->country_code[2] = '\0';

  // Parse coordinates
  parse_coordinates(raw->coordinates, &processed->latitude,
                    &processed->longitude);

  // Parse function code
  parse_function_code(raw->function_code, &processed->is_port,
                      &processed->is_airport, &processed->is_train_station);
}


char *escape_csv_field(const char *field, char *buffer, size_t buffer_size) {
  if (!field || strlen(field) == 0) {
    strncpy(buffer, "\"\"", buffer_size);
    return buffer;
  }

  // Check if we need to escape
  bool needs_escape = false;
  for (const char *p = field; *p; p++) {
    if (*p == '"' || *p == ',' || *p == '\n' || *p == '\r') {
      needs_escape = true;
      break;
    }
  }

  if (!needs_escape) {
    strncpy(buffer, field, buffer_size);
    return buffer;
  }

  size_t j = 0;
  buffer[j++] = '"';
  for (const char *p = field; *p && j < buffer_size - 2; p++) {
    if (*p == '"') {
      buffer[j++] = '"';
    }
    buffer[j++] = *p;
  }
  buffer[j++] = '"';
  buffer[j] = '\0';

  return buffer;
}




