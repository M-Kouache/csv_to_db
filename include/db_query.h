#ifndef DB_QUERY_H
#define DB_QUERY_H

#include <libpq-fe.h>
#include "parsers.h"

void create_table(PGconn *conn);

bool batch_insert_locations(PGconn *conn, ProcessedLocation *locations,
                            int count);

#endif
