#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tarantool/tarantool.h>
#include <tarantool/tnt_net.h>
#include <tarantool/tnt_opt.h>
#include <tarantool/tnt_schema.h>
#include <msgpuck.h>

int store_data_into_tarantool(struct tnt_stream* stream, int space_id, char* uuid, char* data);
char* load_data_from_tarantool(struct tnt_stream* stream, int space_id, char* uuid);
void disconnect_from_tarantool(struct tnt_stream * tnt);
