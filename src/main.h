#pragma once
#include "libpq-fe.h"
#include "tarantool/tarantool.h"

void handle_get_request(PGconn* conn, struct tnt_stream * stream, int client_socket, char* psql_tb_name, int tarantool_space_id, char* uuid);

void handle_put_request(PGconn* conn, struct tnt_stream * stream, int client_socket, char* psql_tb, int tarantool_space_id, char* json_payload_buffer);


