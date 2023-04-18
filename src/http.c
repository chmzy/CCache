#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "http.h"
#include "error.h"

#define LISTEN_CONN_NUM 6
#define HTTP_RESPONSE_SIZE 1024

const char* status2str(enum http_status status)
{
        const char* strings[] = {
            [HTTP_OK] = "Ok",
            [HTTP_BAD_REQUEST] = "Bad Request",
            [HTTP_NOT_FOUND] = "Not found",
            [HTTP_METHOD_NOT_ALLOWED] = "Method not allowed",
            [HTTP_INTERNAL_SERVER_ERROR] = "Internal server error",
        };
        return strings[status];
}

int create_listening_socket(unsigned short port)
{       
    // Initialize variables
    int server_socket = -1;
    int option = 1;
    
    // Set server address parameters
    struct sockaddr_in server_address = {
        .sin_family = AF_INET,
        .sin_port = htons(port),
        .sin_addr = {INADDR_ANY},
    };
    
    // Initialize socket
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        error_handler("create socket failed");
        //perror("create socket failed");
        //exit(EXIT_FAILURE);
    }
    
    /*Setting the SO_REUSEADDR option allows a socket to be created 
    and bound immediately after closing a previous socket on that port.  
    Speeds up program restart during debugging.
    */
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option)) < 0) {
         error_handler("setsockopt failed");
    }

    // Bind socket to server adress
    if (bind(server_socket, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
        error_handler("bind socket failed");
    }

    // Listen to incoming connections
    if (listen(server_socket, LISTEN_CONN_NUM) < 0) {
        error_handler("listen socket failed");
    }

    return server_socket;
}

char* create_http_response(enum http_status status, char type[], char body[])
{
    
   char* response = (char*)malloc(HTTP_RESPONSE_SIZE*sizeof(char));
   
   if (!response){
    error_handler("Failed to malloc memory for response in function create_http_response");
   }

    snprintf(response, HTTP_RESPONSE_SIZE, "HTTP/1.1 %d %s\r\n"
                                            "Content-Type: %s\r\n"
                                            "\r\n"
                                            "%s\r\n",
                status, status2str(status), type, body);

    return response;
}

char* parse_http_json(char* req_buf){
        // Initialize JSON payload buffer
        char* json_payload_start = strchr(req_buf, '{');
        char* json_payload_end = strrchr(req_buf, '}');
        
        if (json_payload_start == NULL || json_payload_end == NULL) {
            
            // If initial http request doesn't contain json payload,
            // Will send empty json payload
            char* json_payload_buffer= (char*)malloc(3*sizeof(char));
            json_payload_buffer[0] = '{';
            json_payload_buffer[1] = '}';
            json_payload_buffer[2] = '\0';
            
            //return string with json
            return json_payload_buffer;  

        } else{

            // if there is json payload in http request
            // Parse it by finding start and end position
            int json_payload_length = json_payload_end - json_payload_start + 1;
            char* json_payload_buffer = (char*)malloc((json_payload_length+1)*sizeof(char));
            strncpy(json_payload_buffer, json_payload_start, json_payload_length);
            json_payload_buffer[json_payload_length] = '\0';
            
            //return string with json
            return json_payload_buffer;
        }
}

http_data_t* parse_http_header(char* request) {
    
    // Allocate memory on the heap for the http_data_t struct.
    http_data_t* http_data = (http_data_t*)malloc(sizeof(http_data_t));

    // Find the end of the first line of the request by searching for the "\r\n"
    // delimiter. This assumes that the HTTP header has only one line, which may
    // not be the case for all requests. If no delimiter is found, return NULL.
    char* end = strstr(request, "\r\n");
    if (end == NULL) {
        return NULL;
    }

    // Replace the "\r\n" delimiter with a null character to terminate the first line.
    *end = '\0';

    // Take first substring before ? character without strtok


    // Parse the first line of the request using sscanf, which reads formatted data
    // from a string. The "%ms" format specifier allocates memory on the heap for a
    // string argument and stores a pointer to it in the corresponding pointer variable.
    sscanf(request, "%ms %ms %ms", &(http_data->method), &(http_data->endpoint), &(http_data->version));

    // Restore the "\r\n" delimiter.
    *end = '\r';

    // Return a pointer to the http_data_t struct.
    return http_data;
}

char* parse_query_param(char* header) {
    // Find the start of the query parameter.
    const char* query_start = strchr(header, '?');
    if (query_start == NULL) {
        fprintf(stderr, "%s", "No query parameters found\n");
        return NULL;
    } 

    // Find the start of the "id" parameter.
    const char* id_start = strstr(query_start, "id=");
    if (id_start == NULL) {
        fprintf(stderr, "%s", "No 'id' parameter found\n");
        return NULL;
    }

    // Extract the value of the "id" parameter.
    const char* value_start = id_start + strlen("id=");
    const char* value_end = strpbrk(value_start, "& ");
    if (value_end == NULL) {
        value_end = value_start + strlen(value_start);
    }
    
    size_t value_len = value_end - value_start;
    
    char* id_str = malloc(value_len + 1);
    if (id_str == NULL) {
        fprintf(stderr, "%s", "Failed to allocate memory\n");
        return NULL;
    }
    strncpy(id_str, value_start, value_len);
    id_str[value_len] = '\0';

    // Return the parsed ID value as a string.
    return id_str;
}