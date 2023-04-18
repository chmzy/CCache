#include "error.h"

#include <stdio.h>
#include <stdlib.h>

void error_handler(char* err_desc){
    fprintf(stderr, err_desc);
    exit(EXIT_FAILURE);
}