#pragma once
#include <libpq-fe.h>

PGconn* connect_to_db(char* user, char* password,  char* dbname);

void disconnect_from_db(PGconn *conn);

PGresult *execute_query(PGconn *conn, char *query);

char* genearate_psql_uuid(PGconn *conn);



