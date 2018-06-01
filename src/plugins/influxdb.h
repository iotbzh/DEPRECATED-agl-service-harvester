/*
 * Copyright (C) 2018 "IoT.bzh"
 * Author "Romain Forlot" <romain.forlot@iot.bzh>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *	 http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _INFLUXDB_H_
#define _INFLUXDB_H_

#define _GNU_SOURCE
#include <string.h>

#include "../utils/list.h"
#include "ctl-plugin.h"
#include "tsdb.h"
#include "wrap-json.h"

struct serie_columns_t {
    struct list* tags;
    struct list* fields;
};

struct series_t {
    const char* name;
    struct serie_columns_t serie_columns;
    uint64_t timestamp;
};

int create_database(AFB_ReqT request);

int unpack_metric_from_api(json_object* m, struct series_t* serie);

static inline int should_escape(char c)
{
    switch (c) {
    case ',':
    case '=':
    case ' ':
    case '"':
        return 1;
        break;
    }
    return 0;
}

static inline char* escape_chr(const char* src)
{
    int j, i = 0;
    size_t len, src_len;
    char* res;

    len = src_len = strlen(src);
    while (i < src_len) {
        if (should_escape(src[i++]))
            len++;
    }

    if (len == src_len) {
        res = strdup(src);
    } else {
        res = malloc(len + 1);
        if (res) {
            i = j = 0;
            while (i < src_len) {
                if (should_escape(src[i]))
                    res[j++] = '\\';
                res[j++] = src[i++];
            }
        }
        res[j] = '\0';
    }
    return res;
}

static inline void concatenate(char* dest, const char* source, const char* sep)
{
    char* esc_source;

    if (sep)
        strncat(dest, sep, strlen(sep));

    esc_source = escape_chr(source);

    strncat(dest, esc_source, strlen(esc_source));

    if (esc_source)
        free(esc_source);
}

size_t make_url(char* url, size_t l_url, const char* host, const char* port, const char* endpoint);

#endif
