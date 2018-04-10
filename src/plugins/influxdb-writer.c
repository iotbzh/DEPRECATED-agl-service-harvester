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
#include <stdio.h>
#include <string.h>
#include "influxdb.h"

size_t format_write_args(char *query, const char *name, const char *source, const char *unit, const char *identity, json_object *jv, uint64_t timestamp)
{
	char *ts;
	memset(query, 0, URL_MAXIMUM_LENGTH);

	strncat(query, name, strlen(name));
	if (source) {
		concatenate(query, source, ",");
	}
	if (unit) {
		concatenate(query, unit, ",");
	}
	if (identity) {
		concatenate(query, identity, ",");
	}

	concatenate(query, "value", " ");
	concatenate(query, json_object_to_json_string(jv), "=");
	asprintf(&ts, "%lu", timestamp);
	concatenate(query, ts, " ");

	return strlen(query);
}

CURL *make_curl_write_post(const char *url, json_object *metric)
{
	CURL *curl;
	json_object *jv = NULL;
	uint64_t timestamp = 0;
	const char *name = NULL,
			*source = NULL,
			*unit = NULL,
			*identity = NULL;

	size_t lpd = json_object_is_type(metric, json_type_array) ?
				 json_object_array_length(metric) + 1 : 2;

	char **post_data;
	post_data = malloc(lpd);

	char write[URL_MAXIMUM_LENGTH];
	bzero(write, URL_MAXIMUM_LENGTH);

	if(unpack_metric_from_binding(metric, &name, &source, &unit, &identity, &jv, &timestamp)) {
		AFB_ERROR("ERROR unpacking metric. %s", json_object_to_json_string(metric));
		curl = NULL;
	}
	else {
		for(long int i = --lpd; i >= 0; i--) {
			format_write_args(write, name, source, unit, identity, jv, timestamp);
			post_data[i] = i == lpd ? NULL : write;
		}
		curl = curl_wrap_prepare_post(url, NULL, 1, (const char * const*)post_data);
	}
	free(post_data);

	return curl;
}

void influxdb_write_curl_cb(void *closure, int status, CURL *curl, const char *result, size_t size)
{
	struct afb_req *req = (struct afb_req*)closure;
	long rep_code = curl_wrap_response_code_get(curl);
	switch(rep_code) {
		case 204:
			AFB_DEBUG("Request correctly written");
			afb_req_success(*req, NULL, "Request has been successfully writen");
			break;
		case 400:
			afb_req_fail(*req, "Bad request", result);
			break;
		case 401:
			afb_req_fail(*req, "Unauthorized access", result);
			break;
		case 404:
			afb_req_fail(*req, "Not found", result);
			AFB_NOTICE("Attempt to create the DB '"DEFAULT_DB"'");
			create_database();
			break;
		case 500:
			afb_req_fail_f(*req, "Timeout", "Overloaded server: %s", result);
			break;
		default:
			afb_req_fail(*req, "Failure", "Unexpected behavior.");
			break;
	}
}

CURL *influxdb_write(const char* host, const char *port, json_object *metric)
{
	char url[URL_MAXIMUM_LENGTH]; /* Safe limit for most popular web browser */
	make_url(url, sizeof(url), host, port, "write");
	return make_curl_write_post(url, metric);
}
