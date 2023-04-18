#include "postgresql.h"
#include "libpq-fe.h"
#include <stdlib.h>

// Function to connect to the database with given parameters

PGconn* connect_to_db(char* user, char* password,  char* dbname)
{

    PGconn *conn;
    char conninfo[1024];
    sprintf(conninfo, "user=%s password=%s dbname=%s", user, password, dbname);
    conn = PQconnectdb(conninfo);
    if (PQstatus(conn) != CONNECTION_OK)
    {

        fprintf(stderr, "Connection to database failed: %s\n", PQerrorMessage(conn));
        PQfinish(conn);

        return NULL;

    }
    return conn;
}


// Function to disconnect from the database
void disconnect_from_db(PGconn *conn)
{
    PQfinish(conn);
    return;
}

// Function to execute query
PGresult* execute_query(PGconn *conn, char *query)
{
    PGresult* res;
    
    res = PQexec(conn, query);
    
    if (PQresultStatus(res) != PGRES_COMMAND_OK )
    {
        fprintf(stderr, "Query failed: %s\n", PQerrorMessage(conn));
        PQclear(res);
        return NULL;
    }
    
    return res;
}

// Function to generate uuid inside postgresql
char* genearate_psql_uuid(PGconn *conn){

    // Generate UUID using PostgreSQL function
    PGresult *res = PQexec(conn, "SELECT uuid_generate_v4()");

    // Check if query was successful
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "Query execution failed: %s %d", PQresultErrorMessage(res), PQresultStatus(res));
        PQclear(res);
        PQfinish(conn);
        return NULL;
    }

    // Get UUID value from result set
    char* uuid = (char*)malloc(sizeof(char) * 37);
    
    uuid = PQgetvalue(res, 0, 0);

    PQclear(res);

    return uuid;
}

