/*
 * Copyright (C) 2015, 2016, 2017, 2018 "IoT.bzh"
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

#include <json-c/json.h>
#include "curl-wrap.h"

enum db_available {
	INFLUX = 1,
	GRAPHITE = 2,
	OPENTSDB = 4
};

CURL *influxdb_write(const char* host, int port, json_object *metric);
CURL *influxdb_read(const char* host, int port, json_object *query);
void influxdb_cb(void *closure, int status, CURL *curl, const char *result, size_t size);
int db_ping();
