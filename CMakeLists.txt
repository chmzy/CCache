cmake_minimum_required(VERSION 3.10)

set(CMAKE_C_COMPILER "gcc")
set(CMAKE_CXX_COMPILER "g++")

project(pico C)

# Add source files
set(SOURCES
    src/main.c
    src/cJSON.c
    src/error.c
    src/config.c
    src/http.c
    src/postgresql.c
    src/tarantool_f.c
)

# Add header files
set(HEADERS
    src/main.h
    src/cJSON.h
    src/error.h
    src/config.h
    src/http.h
    src/postgresql.h
    src/tarantool_f.h
)

# Add libraries
add_library(cJSON STATIC src/cJSON.c)
add_library(error STATIC src/error.c)
add_library(config STATIC src/config.c)
add_library(http STATIC src/http.c)
add_library(postgresql STATIC src/postgresql.c)
add_library(tarantool_f STATIC src/tarantool_f.c)

# Add the PostgreSQL library
find_library(LIBPQ pq)

# Add the tarantool library
find_library(TARANTOOL_LIB tarantool)

# Add executable
add_executable(pico ${SOURCES} ${HEADERS})

# Link libraries
target_link_libraries(pico cJSON error config ${LIBPQ} postgresql ${TARANTOOL_LIB} tarantool_f)
