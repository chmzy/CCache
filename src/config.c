#include "config.h"

#define CONFIG_MAX_LINE_LEN 128
#define DELIM "="

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
) {
    FILE *config_file = fopen(filename, "r");
    if (config_file == NULL) {
        fprintf(stderr, "Error opening config file\n");
        return 1;
    }

    char line[CONFIG_MAX_LINE_LEN];
    while (fgets(line, CONFIG_MAX_LINE_LEN, config_file)) {
        // Ignore comments and empty lines
        if (line[0] == '#' || line[0] == '\n') {
            continue;
        }

        // Split the line into key-value pair
        char *key = strtok(line, DELIM);
        char *val = strtok(NULL, DELIM);

        // Trim leading/trailing whitespace from key and value
        key = strtok(key, " \t\n");
        val = strtok(val, " \t\n");

        // Use key-value pair to configure your program
        if (strcmp(key, "SOCKET_PORT") == 0) {
            *port_socket = atoi(val);
        } else if (strcmp(key, "PSQL_USERNAME") == 0) {
            strncpy(psql_username, val, strlen(val)+1);
        } else if (strcmp(key, "PSQL_PASSWORD") == 0) {
            strncpy(psql_password, val, strlen(val)+1);
        } else if (strcmp(key, "PSQL_DBNAME") == 0) {
            strncpy(psql_dbname, val, strlen(val)+1);
        } else if (strcmp(key, "PSQL_TBNAME") == 0) {
            strncpy(psql_tbname, val, strlen(val)+1);
        } else if (strcmp(key, "TARANTOOL_USERNAME") == 0) {
            strncpy(tarantool_username, val, strlen(val)+1);
        } else if (strcmp(key, "TARANTOOL_PASSWORD") == 0) {
            strncpy(tarantool_password, val, strlen(val)+1);
        } else if (strcmp(key, "TARANTOOL_HOST") == 0) {
            strncpy(tarantool_host, val, strlen(val)+1);
        } else if (strcmp(key, "TARANTOOL_PORT") == 0) {
            strncpy(port_tarantool, val, strlen(val)+1);
        } else if (strcmp(key, "TARANTOOL_SPACE_ID") == 0) {
            *tarantool_space_id = atoi(val);
        } else {
            printf("Unknown key: %s\n", key);
        }
    }

    fclose(config_file);
    return 0;
}

int file_exists(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (file != NULL) {
        fclose(file);
        return 0;
    } else {
        return -1;
    }
}

char* parse_config_path(int argc, char *argv[]) {
    
    if (argc < 2){
        printf("Usage: -f path/to/config.txt \n");
        return NULL;
    }

    char *filename = NULL;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-f") == 0 && i + 1 < argc) {
            filename = argv[i + 1];
            break;
        }
    }

    if (filename != NULL && strstr(filename, ".txt") != NULL && file_exists(filename) == 0) {
        return filename;
    } else {
        printf("Usage: -f path/to/config.txt \n");
        return NULL;
    }
}
