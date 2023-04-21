#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "http.h"
#include "error.h"
#include "config.h"
#include "cJSON.h"
#include "postgresql.h"
#include "tarantool_f.h"

#define REQUEST_BUF_SIZE 1024
#define SQL_QUERY_SIZE 1024
#define LISTEN_CONN_NUM 6

#define DEBUG_MODE 1


void handle_get_request(PGconn* conn, struct tnt_stream * stream, int client_socket, char* psql_tb_name, int tarantool_space_id, char* uuid) {
    
    // Notify handling method
    if(DEBUG_MODE == 1){
        printf("New GET request!\n");
    }

    //Check if tarantool in reachable
    if(tnt_connect(stream) < 0){
            printf("GET: Error connecting to Tarantool!\n");
            goto postgresql; //If not -> get from postgresql
    }

    // Ask tarantool for data
    char* request = load_data_from_tarantool(stream, tarantool_space_id, uuid);
    
    // Check if data exists in Tarantool Cache
    if (request == NULL) {
        printf("GET: Error recieving data from tarantool\n");
        return;
    }
    
    //UUID_NF is error code for not found in tarantool cache
    if (strcmp(request, "UUID_NF") != 0) {

        printf("Found in Tarantool\n");
        
        cJSON* json_response = cJSON_CreateObject();
        cJSON_AddStringToObject(json_response, "id", uuid);
        cJSON_AddStringToObject(json_response, "data", request);

        // Create HTTP response
        char* response = create_http_response(HTTP_OK, "application/json", cJSON_Print(json_response));

        // Send HTTP response to client
        send(client_socket, response, strlen(response), 0);
        
        // Free memory
        free(response);
        cJSON_Delete(json_response);
        free(request);
        return;

    } else{
    postgresql:
        printf("UUID not found in tarantool cache. Getting from PostgreSQL\n");

        // Check if connection to database is OK
        if (PQstatus(conn) != CONNECTION_OK) {
            fprintf(stderr, "Connection to database failed: %s\n", PQerrorMessage(conn));
            PQfinish(conn);
            return;
        }
        
        // Execute SQL query to get data from database with primary key uuid
        PGresult *res; 
        char sql_query[SQL_QUERY_SIZE];
        sprintf(sql_query, "SELECT data FROM %s WHERE id = '%s'", psql_tb_name, uuid);

        // Check if uuid presents in database
        if ( PQresultStatus(res = PQexec(conn, sql_query)) != PGRES_TUPLES_OK){
            
            fprintf(stderr, "Error recieving data from PSQL %s!\n %s\n", psql_tb_name, PQresultErrorMessage(res));
            char* response = create_http_response(HTTP_NOT_FOUND, "text/plain", PQresultErrorMessage(res));
            send(client_socket, response, strlen(response), 0);
            free(response);
            PQclear(res);
            return;
        }

        // Check if any rows were returned
        if (PQntuples(res) == 0) {
            printf("No rows found with uuid %s in table %s\n", uuid, psql_tb_name);
            // Create HTTP response
            char* response = create_http_response(HTTP_NOT_FOUND, "text/pain", "No rows found with uuid");

            // Send HTTP response to client
            send(client_socket, response, strlen(response), 0);

            // Free memory
            free(response);
            PQclear(res);
            return;
        } else {
          // Iterate over the rows and print the data column
            char* data = PQgetvalue(res, 0, 0);
            cJSON* json_response = cJSON_CreateObject();
            cJSON_AddStringToObject(json_response, "id", uuid);
            cJSON_AddStringToObject(json_response, "data", data);

            // Create HTTP response
            char* response = create_http_response(HTTP_OK, "application/json", cJSON_Print(json_response));

            // Send HTTP response to client
            send(client_socket, response, strlen(response), 0);

            // Free memory
            free(response);
            cJSON_Delete(json_response);
            PQclear(res);
            return;
        }
    }

}

void handle_put_request(PGconn* conn, struct tnt_stream * stream, int client_socket, char* psql_tb_name, int tarantool_space_id, char* json_payload_buffer){
    
    // Notify handling method
    if(DEBUG_MODE == 1){
        printf("New PUT request!\n");
    }

    // Parse JSON payload from buffer
    cJSON* json_payload = cJSON_Parse(json_payload_buffer);
    if (json_payload == NULL) {
        printf("Error parsing JSON. Maybe it is not a valid JSON?\n");
        return;
    }

    // Get the "data" field from the root object
    cJSON* data_field = cJSON_GetObjectItemCaseSensitive(json_payload, "data");
    if (data_field == NULL) {
        printf("Error parsing data field from JSON payload. Maybe this filed in not in the JSON payload?\n");
        cJSON_Delete(json_payload);
        return;
        
    }
    // Check if the "data" field is a string
    if (!cJSON_IsString(data_field)) {
        printf("Data field of JSON payload is not a string.\n");
        cJSON_Delete(json_payload);
        return;
    }

    // Check if connection to database is OK
        if (PQstatus(conn) != CONNECTION_OK) {
            fprintf(stderr, "Connection to database failed: %s\n", PQerrorMessage(conn));
            PQfinish(conn);
            return;
        }
    
    // Generate uuid inside postgresql
    PGresult *res; 

    res = PQexec(conn, "SELECT uuid_generate_v4()");

    // Check if uuid was generated
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        
        char* response = create_http_response(HTTP_BAD_REQUEST, "text/plain", PQerrorMessage(conn));
        
        // Send HTTP response to client
        send(client_socket, response, strlen(response), 0);

        // Free memory
        free(response);
        PQclear(res);
        PQfinish(conn);
        return;
  }

    printf("UUID result: %d \n", PQresultStatus(res));
    char* uuid = PQgetvalue(res, 0, 0);

    int uuid_length = strlen(uuid);
    if (uuid_length != 36) {
        fprintf(stderr, "Unexpected UUID length: %d", uuid_length);
        char* response = create_http_response(HTTP_BAD_REQUEST, "text/plain", "Unexpected UUID length");
        
        // Send HTTP response to client
        send(client_socket, response, strlen(response), 0);

        // Free memory
        free(response);
        PQclear(res);
        PQfinish(conn);
        return;
  }

    // Store json data field value in database with primary key uuid
    char sql_query[SQL_QUERY_SIZE];

    char* data_field_str = (char*)malloc(strlen(cJSON_GetStringValue(data_field))+1);
    strcpy(data_field_str, cJSON_GetStringValue(data_field));
    
    
    sprintf(sql_query, "INSERT INTO %s (id, data) VALUES ('%s', '%s')", psql_tb_name, uuid, data_field_str);

    printf("SQL query: %s\n", sql_query);

    // Execute SQL query
    if ( PQresultStatus(res = PQexec(conn, sql_query)) == PGRES_FATAL_ERROR){
        
        fprintf(stderr, "Error inserting data into table %s!\n %s\n", psql_tb_name, PQresultErrorMessage(res));

        // Create HTTP response with generated uuid
        char* response = create_http_response(HTTP_INTERNAL_SERVER_ERROR, "text/plain",PQresultErrorMessage(res));

        // Send HTTP response to client
        send(client_socket, response, strlen(response), 0);

        // Free memory
        free(response);
        free(data_field_str);
        PQclear(res);
        PQfinish(conn);
        
        return;

    } else{
        
        printf("Data inserted into PSQL table %s!\n", psql_tb_name);
        // Create json payload with field "data" and "uuid" using cJSON
        cJSON* json_response = cJSON_CreateObject();
        cJSON_AddStringToObject(json_response, "id", uuid);

        // Create HTTP response with generated uuid
        char* response = create_http_response(HTTP_OK, "application/json", cJSON_Print(json_response));

        // Send HTTP response to client
        send(client_socket, response, strlen(response), 0);

        // Free memory
        cJSON_Delete(json_payload);
        cJSON_Delete(json_response);
        free(response);
        PQclear(res);
        
    }

    if(tnt_connect(stream) < 0){
        printf("PUT: Error connecting to Tarantool!\n");
        char* response = create_http_response(HTTP_INTERNAL_SERVER_ERROR, "text/plain", "PUT: Error connecting to Tarantool!\n");
        send(client_socket, response, strlen(response), 0);
        free(response);
        free(data_field_str);
        return;
    } else{   
    // Store json data field value in tarantool cache database as (str, str)
        if (store_data_into_tarantool(stream, tarantool_space_id, uuid, data_field_str) < 0){
            fprintf(stderr, "PUT: Error inserting data %s into tarantool cache!\n", data_field_str);
            
            free(data_field_str);
            return;
            
        }else{
            printf("PUT: Data %s inserted into Tarantool cache!\n", data_field_str);
            
            free(data_field_str);
            
            return;
        }
    
    }
}

int main(int argc, char* argv) {

    /****************************************************************/
    // Parse config file
    char* config_path;
    int port_socket = 0;
    char psql_username[CONFIG_MAX_LINE_LEN];
    char psql_password[CONFIG_MAX_LINE_LEN];
    char psql_dbname[CONFIG_MAX_LINE_LEN];
    char psql_tbname[CONFIG_MAX_LINE_LEN];
    char tarantool_username[CONFIG_MAX_LINE_LEN];
    char tarantool_password[CONFIG_MAX_LINE_LEN];
    char tarantool_host[CONFIG_MAX_LINE_LEN];
    char port_tarantool[CONFIG_MAX_LINE_LEN];
    int tarantool_space_id = 0;
    
    if ((config_path = parse_config_path(argc, argv)) == NULL){
        error_handler("Error parsing flags!\n");
    }

    if (parse_config_file(config_path, 
                          &port_socket, 
                          psql_username, 
                          psql_password, 
                          psql_dbname, 
                          psql_tbname, 
                          tarantool_username, 
                          tarantool_password, 
                          tarantool_host, 
                          port_tarantool, 
                          &tarantool_space_id) != 0) {
       error_handler("Error parsing config file!\n");
    }

    /****************************************************************/
    // Connect to PostgreSQL database
    PGconn* psql_conn = connect_to_db(psql_username, psql_password, psql_dbname);
    
    // Check PostgreSQL connection
    if (PQstatus(psql_conn) != CONNECTION_OK) {
        // Notify about PSQL database connection fail
        fprintf(stderr, "Connection to database failed: %s\n", PQerrorMessage(psql_conn));
        PQfinish(psql_conn);
        exit(EXIT_FAILURE);
    } else{
        // Notify about PSQL database connected successfully
        printf("PostgreSQL database connected!\n");
    }

    // Intialize PSQL reuslt variable and SQL query array
    PGresult* res;
    char sql_query[SQL_QUERY_SIZE];
    sprintf(sql_query, "SELECT * FROM %s", psql_tbname);
    
    // Check if table, where data will be stored, in database aleady exists
    if ( PQresultStatus(res = PQexec(psql_conn, sql_query)) == PGRES_FATAL_ERROR ){
        printf("Table %s does not exist!\n", psql_tbname);
        
        // Create table with uuid as primary key
        sprintf(sql_query, "CREATE TABLE %s (id uuid PRIMARY KEY, data text)", psql_tbname);

        printf("SQL query: %s\n", sql_query);

        if ( PQresultStatus(res = PQexec(psql_conn, sql_query)) == PGRES_FATAL_ERROR){
            printf("Error creating table %s!\n", psql_tbname);
            return 1;
        } else{

            printf("Table %s created!\n", psql_tbname);
        }
    } else{
        printf("Table %s already exists!\n", psql_tbname);
    }

    /****************************************************************/
    // Connect to Tarantool database
    struct tnt_stream* tnt = tnt_net(NULL);
    char* tnt_uri = (char*)malloc((strlen(tarantool_username) + strlen(tarantool_password) + strlen(tarantool_host) + strlen(port_tarantool) + 4));
    sprintf(tnt_uri, "%s:%s@%s:%s", tarantool_username, tarantool_password, tarantool_host, port_tarantool);
    tnt_set(tnt, TNT_OPT_URI, tnt_uri);

    //tnt_set(tnt, TNT_OPT_URI, "admin:pass@localhost:8081");
    
    if (tnt_connect(tnt) < 0) {                      
        struct tnt_reply reply;  
        tnt_reply_init(&reply); 
        tnt->read_reply(tnt, &reply);
        printf("Error connecting to Tarantool!\n");
        tnt_reply_free(&reply);

        return 1;
    }

    // Notify about Tarantool database connection
    printf("Tarantool database connected! Running of port %s\n", port_tarantool);
    /****************************************************************/

    // Initial variables for listening socket server
    int server_socket = -1;
    int client_socket = -1;
    char request_buffer[REQUEST_BUF_SIZE];
    struct sockaddr_in client_address;
    socklen_t client_address_length;
    client_address_length = sizeof(client_address);

    // Start new listenig server
    // No error checks needed because they have been already done inside function
    server_socket = create_listening_socket(port_socket);

    // Notify about server starting
    printf("Server is running on port %d\n", port_socket);
    /****************************************************************/

    //This while loop condition of handling socket connections allows program 
    //to perform other tasks while waiting for new connections, 
    //and handles new connections as they arrive.
    while ((client_socket = accept(server_socket, (struct sockaddr *)&client_address, (socklen_t*)&client_address_length)) > 0) {
        
        // Receive incoming HTTP request
        if ((recv(client_socket, request_buffer, REQUEST_BUF_SIZE, 0)) < 0){
            fprintf(stderr, "Error recieving data from client");
            continue;
        }

        // Parse HTTP data such as method, endpoint and version
        http_data_t* http_header = parse_http_header(request_buffer);
        
        printf("ENDPOINT: %s\n", http_header->endpoint);

        // Parse query parameter from http_header->endpoint
        char* query_param = parse_query_param(http_header->endpoint);

        printf("QUERY PARAMETER: %s\n", query_param);

        // Parse HTTP json payload
        char* json_payload_buffer = parse_http_json(request_buffer);  

        printf("JSON PAYLOAD: %s\n", json_payload_buffer);

        // Handle HTTP methods
        
        // Check if HTTP method is PUT
        if(strcmp(http_header->method, "PUT") == 0) {
            
            // Check if endpoint /data has been requested 
            if(strcmp(http_header->endpoint, "/data") == 0){
                
                // Proceed to PUT handler
                handle_put_request(psql_conn, tnt, client_socket, psql_tbname, tarantool_space_id, json_payload_buffer);

            } else {
                // If endpoint is not /data, return HTTP 400
                char* response = create_http_response(HTTP_BAD_REQUEST, "text/plain"," Endpoint not found");
            
                send(client_socket, response, strlen(response), 0);
            
                free(response);
            }

        // Check if HTTP method is GET
        } else if(strcmp(http_header->method, "GET") == 0) {

            // Check if query parameter is present in HTTP request
            if (query_param != NULL) {
                
                // Check if endpoint /data has been requested
                if (strncmp(http_header->endpoint, "/data", strcspn(http_header->endpoint, "?")) == 0){
                    
                    // Proceed to GET handler
                    handle_get_request(psql_conn, tnt, client_socket,  psql_tbname, tarantool_space_id, query_param);

                } else {
                    char* response = create_http_response(HTTP_NOT_FOUND, "text/plain","Endpoint not found");
            
                    send(client_socket, response, strlen(response), 0);

                    free(response); 
                }

            } else {
                char* response = create_http_response(HTTP_BAD_REQUEST, "text/plain"," Query parameter not found");
            
                send(client_socket, response, strlen(response), 0);
            
                free(response); 
            }

        } else{ 
            // If HTTP method is neither GET nor PUT, return HTTP 400
            char* response = create_http_response(HTTP_BAD_REQUEST, "text/plain"," Unsupported HTTP method");
            
            send(client_socket, response, strlen(response), 0);
            
            free(response); 
        }

        
        // Free request_buffer
        memset(request_buffer, 0, sizeof(request_buffer));

        // Free json_payload_buffer
        free(json_payload_buffer);
        
        // Free http_header
        free(http_header);

        // Free query_param
        free(query_param);

        // Close client socket
        close(client_socket);
    }
    
    // Close server socket
    close(server_socket);

    // Close PostgreSQL database connection
    disconnect_from_db(psql_conn);

    //Close Tarantool database connection
    disconnect_from_tarantool(tnt);
    
    free(tnt_uri);
    
    return 0;

}