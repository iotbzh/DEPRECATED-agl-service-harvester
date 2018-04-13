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

#ifndef _TSDB_H
#define _TSDB_H

#define AFB_BINDING_VERSION 2
#include <afb/afb-binding.h>

#include <json-c/json.h>
#include "curl-wrap.h"

#define ERROR -1

#define DEFAULT_DB "agl-garner"
#define DEFAULT_DBHOST "localhost"
#define DEFAULT_DBPORT "8086"
#define URL_MAXIMUM_LENGTH 2047

enum db_available {
	INFLUX = 1,
	GRAPHITE = 2,
	OPENTSDB = 4
};

struct reader_args {
	const char *host;
	const char *port;
	u_int32_t delay;
};

CURL *influxdb_write(const char* host, const char *port, json_object *metric);
void influxdb_write_curl_cb(void *closure, int status, CURL *curl, const char *result, size_t size);

int influxdb_reader(void *args);

int db_ping();

u_int64_t get_ts();

#endif
