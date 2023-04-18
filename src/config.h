#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define CONFIG_MAX_LINE_LEN 128

int parse_config_file(
    const char* filename,
    int* port_socket,
    char* psql_username,
    char* psql_password,
    char* psql_dbname,
    char* psql_tbname,
    char* tarantool_username,
    char* tarantool_password,
    char* tarantool_host,
    char* port_tarantool,
    int* tarantool_space_id
);

int file_exists(const char *filename);

char* parse_config_path(int argc, char *argv[]);


