#include "db_query.h"
#include <libpq-fe.h>
#include <stdbool.h>
#include <string.h>


// Create the PostGIS table
void create_table(PGconn *conn) {
  PGresult *res;
  const char *query = "CREATE TABLE IF NOT EXISTS locations (\
    id SERIAL PRIMARY KEY,\
    unlocode VARCHAR(10) NOT NULL,\
    name TEXT NOT NULL, \
    country_code CHAR(2) NOT NULL, \
    location GEOGRAPHY(POINT, 4326),\
    is_airport BOOLEAN DEFAULT FALSE,\
    is_port BOOLEAN DEFAULT FALSE,\
    is_train_station BOOLEAN DEFAULT FALSE,\
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP\
    );";

  res = PQexec(conn, query);
  if (PQresultStatus(res) != PGRES_COMMAND_OK) {
    fprintf(stderr, "Table creation failed: %s", PQerrorMessage(conn));
    PQclear(res);
    return (void)NULL;
  }
  PQclear(res);

  res = PQexec(conn, "CREATE UNIQUE INDEX locations_unlocode_name_idx ON "
                     "locations (unlocode, MD5(name)) WHERE name IS NOT NULL;");
  if (PQresultStatus(res) != PGRES_COMMAND_OK) {
    fprintf(stderr, "Table creation failed: %s", PQerrorMessage(conn));
    PQclear(res);
    return (void)NULL;
  }
  PQclear(res);
}


bool batch_insert_locations(PGconn *conn, ProcessedLocation *locations,
                            int count) {
  PGresult *res;
  res = PQexec(conn, "BEGIN");
  if (PQresultStatus(res) != PGRES_COMMAND_OK) {
    fprintf(stderr, "BEGIN failed: %s", PQerrorMessage(conn));
    PQclear(res);
    return false;
  }
  PQclear(res);

  res = PQexec(conn, "CREATE TEMP TABLE temp_locations (LIKE locations "
                     "INCLUDING ALL) ON COMMIT DROP;");
  if (PQresultStatus(res) != PGRES_COMMAND_OK) {
    fprintf(stderr, "Create temp table failed: %s", PQerrorMessage(conn));
    PQclear(res);
    PQexec(conn, "ROLLBACK");
    return false;
  }
  PQclear(res);

  // Start COPY operation
  const char *copy_cmd =
      "COPY temp_locations(unlocode, name, country_code, location, is_airport, "
      "is_port, is_train_station) FROM STDIN WITH (FORMAT csv)";
  res = PQexec(conn, copy_cmd);
  if (PQresultStatus(res) != PGRES_COPY_IN) {
    fprintf(stderr, "COPY command failed: %s", PQerrorMessage(conn));
    PQclear(res);
    PQexec(conn, "ROLLBACK");
    return false;
  }
  PQclear(res);

  // Buffer for forming CSV lines
  char line[4096];
  char name_buffer[1024];
  char unlocode_buffer[32];

  // Write each location as a CSV line
  for (int i = 0; i < count; i++) {
    // Skip records with empty names
    if (strlen(locations[i].name) == 0) {
      continue;
    }

    // Validate coordinates
    if (locations[i].longitude < -180 || locations[i].longitude > 180 ||
        locations[i].latitude < -90 || locations[i].latitude > 90) {
      // Skip invalid coordinates or set to default
      continue;
    }
    // Format the point in PostGIS format
    snprintf(
        line, sizeof(line), "%s,%s,%s,\"SRID=4326;POINT(%f %f)\",%s,%s,%s\n",
        escape_csv_field(locations[i].unlocode, unlocode_buffer,
                         sizeof(unlocode_buffer)),
        escape_csv_field(locations[i].name, name_buffer, sizeof(name_buffer)),
        locations[i].country_code,
        locations[i].longitude, // PostGIS expects longitude first
        locations[i].latitude, locations[i].is_airport ? "t" : "f",
        locations[i].is_port ? "t" : "f",
        locations[i].is_train_station ? "t" : "f");

    // Send the line to PostgreSQL
    if (PQputCopyData(conn, line, strlen(line)) != 1) {
      fprintf(stderr, "Put copy data failed: %s", PQerrorMessage(conn));
      PQputCopyEnd(conn, "Error while copying data");
      PQexec(conn, "ROLLBACK");
      return false;
    }
  }

  if (PQputCopyEnd(conn, NULL) != 1) {
    fprintf(stderr, "Put copy end failed: %s", PQerrorMessage(conn));
    PQexec(conn, "ROLLBACK");
    return false;
  }

  res = PQgetResult(conn);
  if (PQresultStatus(res) != PGRES_COMMAND_OK) {
    fprintf(stderr, "COPY failed: %s", PQerrorMessage(conn));
    PQclear(res);
    PQexec(conn, "ROLLBACK");
    return false;
  }
  PQclear(res);

  // Insert from temp table to main table
  res = PQexec(
      conn,
      "INSERT INTO locations(unlocode, name, country_code, "
      "location, is_airport, is_port, is_train_station) "
      "SELECT unlocode, name, country_code, location, "
      "is_airport, is_port, is_train_station "
      "FROM temp_locations "
      "ON CONFLICT (unlocode, MD5(name)) WHERE name IS NOT NULL DO NOTHING");

  if (PQresultStatus(res) != PGRES_COMMAND_OK) {
    fprintf(stderr, "Insert failed: %s", PQerrorMessage(conn));
    PQclear(res);
    PQexec(conn, "ROLLBACK");
    return false;
  }
  PQclear(res);

  // Commit transaction
  res = PQexec(conn, "COMMIT");
  if (PQresultStatus(res) != PGRES_COMMAND_OK) {
    fprintf(stderr, "COMMIT failed: %s", PQerrorMessage(conn));
    PQclear(res);
    return false;
  }
  PQclear(res);

  return true;
}
