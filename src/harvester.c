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

#define _GNU_SOURCE
#include "harvester.h"
#include "harvester-apidef.h"
#include <pthread.h>

#include "plugins/tsdb.h"
#include "curl-wrap.h"
#include "wrap-json.h"

#define ERROR -1

CURL* (*tsdb_write)(const char* host, int port, json_object *metric);
CURL* (*tsdb_read)(const char* host, int port, json_object *metric);
void (*curl_cb)(void *closure, int status, CURL *curl, const char *result, size_t size);
pthread_mutex_t db_mutex;

int do_write(struct afb_req req, const char* host, int port, json_object *metric)
{
	CURL *curl_request;

	curl_request = tsdb_write(host, port, metric);

	pthread_mutex_lock(&db_mutex);
	curl_wrap_do(curl_request, curl_cb, &req);
	pthread_mutex_unlock(&db_mutex);

	return 0;
}

void write(struct afb_req req)
{
	int port = -1;
	const char *host = NULL;

	json_object *req_args = afb_req_json(req),
				*metric = NULL;

	if(wrap_json_unpack(req_args, "{s?s,s?i,so!}",
								"host", &host,
								"port", &port,
								"metric", &metric) || ! metric)
		afb_req_fail(req, "Failed", "Error processing arguments. Miss metric\
		JSON object or malformed");
	else if(do_write(req, host, port, metric))
		afb_req_fail(req, "Failed", "Error processing metric JSON object.\
		Malformed !");
}

void auth(struct afb_req request)
{
	afb_req_session_set_LOA(request, 1);
	afb_req_success(request, NULL, NULL);
}

int init()
{
	int tsdb_available = 0;
	if(curl_global_init(CURL_GLOBAL_DEFAULT) != 0) {
		AFB_ERROR("Something went wrong initiliazing libcurl. Abort");
		return ERROR;
	}

	if(pthread_mutex_init(&db_mutex, NULL) != 0) {
		AFB_ERROR("Something went wrong initiliazing mutex. Abort");
		return ERROR;
	}

	tsdb_available = db_ping();
	switch (tsdb_available) {
		case INFLUX:
			tsdb_write = influxdb_write;
			tsdb_read = influxdb_read;
			curl_cb = influxdb_cb;
			break;
		default:
			AFB_ERROR("No Time Series Database found. Abort");
			return ERROR;
			break;
	}

	return 0;
}
