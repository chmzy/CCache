#pragma once

/* HTTP status codes.
*/
enum http_status
{
        HTTP_OK = 200,
        HTTP_BAD_REQUEST = 400,
        HTTP_NOT_FOUND = 404,
        HTTP_METHOD_NOT_ALLOWED = 405,
        HTTP_INTERNAL_SERVER_ERROR = 501,
};



/* Struct contains HTTP header.
*/
typedef struct HTTP_DATA {
        char* method;
        char* endpoint;
        char* version;
} http_data_t;

/* Function returns string decode of HTTP code status.
*/
const char* status2str(enum http_status status);

 /* Function creates a streaming network socket, binds it to all system
 * address (special INADDR_ANY value and port @port, then sets
 * socket to listen mode. 
 * System calls socket(), bind(), listen() are used.
 */
int create_listening_socket(unsigned short port);

/* Function generates an HTTP response.
 * The response consists of a status string, a Content-Type header with a predefined
 * value text/plain and the body. 
 * send() and snprintf() functions are used.
 */
char* create_http_response(enum http_status status, char type[], char body[]);

/* Function returns json payload parsed from HTTP request 
 * either empty json body if request doesn't contain any payload.
 * strchr() and malloc() functions are used.
 */
char* parse_http_json(char* request_buffer);

/* Function returns HTTP version, endpoint, HTTP method from HTTP request.
 * sscanf() and malloc() functions are used.
 */
http_data_t* parse_http_header(char* req_buf);


/* Function returns value of query parameter id from HTTP request endpoint.
 * strchr(), strpbrk(), strcmp() and malloc() functions are used.
 */
char* parse_query_param(char* endpoint);