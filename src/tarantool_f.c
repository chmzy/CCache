#include "tarantool_f.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int store_data_into_tarantool( struct tnt_stream * stream, int space_id, char* uuid, char* data){
   
    struct tnt_stream *tuple = tnt_object(NULL);     
    tnt_object_format(tuple, "[%s%s]", uuid, data);
    tnt_insert(stream,  space_id, tuple);                     
    tnt_flush(stream);
    struct tnt_reply reply;  tnt_reply_init(&reply); 
    stream->read_reply(stream, &reply);
    if (reply.code != 0) {
       printf("Insert failed %lu\n %s\n", reply.code, reply.error);
       return -1;

    }

    //printf("%lu\n %s\n %s\n", reply.code, reply.error, reply.data);
    printf("Inserted data %s to space %d\n", data, space_id);
                          
    tnt_stream_free(tuple);
    
    return 0;
}

char* load_data_from_tarantool( struct tnt_stream * stream, int space_id, char* uuid){ 

    struct tnt_stream *tuple = tnt_object(NULL);
    tnt_object_format(tuple, "[%s]", uuid); /* кортеж tuple = ключ для поиска */
    tnt_select(stream, space_id, 0, (2^32) - 1, 0, 0, tuple);
    tnt_flush(stream);
    struct tnt_reply reply; tnt_reply_init(&reply);
    stream->read_reply(stream, &reply);
    if (reply.code != 0) {
        printf("Select failed.\n");
        printf("Code: %lu Error: %s\n", reply.code, reply.error);
        return NULL;
    }
    char field_type;
    field_type = mp_typeof(*reply.data);
    if (field_type != MP_ARRAY) {
        printf("no tuple array\n");
        NULL;
    }
    long unsigned int row_count;
    uint32_t tuple_count = mp_decode_array(&reply.data);
    if (tuple_count == 0) { 
        printf("Tarantool: This uuid  does not exist\n");
        return "UUID_NF";
    }

    unsigned int i, j;
    for (i = 0; i < tuple_count; ++i) {
        field_type = mp_typeof(*reply.data);
        if (field_type != MP_ARRAY) {
            printf("Tarantool: no field array\n");
            exit(1);
        }
        uint32_t field_count = mp_decode_array(&reply.data);
        printf("field count=%u\n", field_count);
        for (j = 0; j < field_count; ++j) {
            field_type = mp_typeof(*reply.data);
            if (field_type == MP_UINT) {
                uint64_t num_value = mp_decode_uint(&reply.data);
                printf("value=%lu.\n", num_value);
            } else if (field_type == MP_STR) {
                const char *str_value;
                uint32_t str_value_length;
                str_value = mp_decode_str(&reply.data, &str_value_length);
                
                size_t remaining_len = str_value_length - 37;
                char* remaining_str = (char*)malloc((remaining_len + 1) * sizeof(char));

                if (remaining_str == NULL) {
                    printf("Error: memory allocation failed.");
                    return NULL;
                }

                //Take characters from str_value starting from 37th position (uuid length)

                char* src = (char*)str_value + 37;
                char* dest = remaining_str;

                while (*src != '\0') {
                    *dest = *src;
                    src++;
                    dest++;
                }

                *dest = '\0';

                return remaining_str;

            } else {
                printf("wrong field type\n");
                return NULL;    
            }
        }
    }

    tnt_stream_free(tuple);
    tnt_reply_free(&reply);
}

void disconnect_from_tarantool(struct tnt_stream * tnt){
    tnt_close(tnt);                                  
    tnt_stream_free(tnt);
    return;
}