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
#include "ctl-plugin.h"
#include "wrap-json.h"
#include "tsdb.h"
#include "../utils/list.h"

struct serie_columns_t {
	struct list *tags;
	struct list *fields;
};

struct series_t {
	const char *name;
	struct serie_columns_t serie_columns;
	uint64_t timestamp;
};

int create_database(AFB_ReqT request);

int unpack_metric_from_api(json_object *m, struct series_t *serie);

void concatenate(char* dest, const char* source, const char *sep);

size_t make_url(char *url, size_t l_url, const char *host, const char *port, const char *endpoint);

#endif
