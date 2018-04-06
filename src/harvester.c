/*
 * Copyright (C) 2015, 2016 "IoT.bzh"
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
#include <string.h>
#include <stdio.h>

#include "curl-wrap.h"
#include "wrap-json.h"

#define DEFAULT_DB "agl-collector"
#define DEFAULT_DBHOST "localhost"
#define DEFAULT_DBPORT "8086"
#define URL_MAXIMUM_LENGTH 2047

int dbping()
{
	int ret = 0;
	char *result;
	size_t result_size;
	CURL *request = curl_wrap_prepare_get("localhost:"DEFAULT_DBPORT"/ping",NULL, NULL);
	curl_wrap_perform(request, &result, &result_size);

	if(curl_wrap_response_code_get(request) != 204) {
		AFB_ERROR("TimeSeries DB not reachable");
		ret = -1;
	}

	curl_easy_cleanup(request);
	return ret;
}

int create_database()
{
	int ret = 0;
	char *result;
	size_t result_size;

	// Declare query to be posted
	const char **post_data;
	post_data = malloc(sizeof(*post_data));
	post_data[0] = "q=CREATE DATABASE \""DEFAULT_DB"\"";
	post_data[1] = NULL;

	CURL *request = curl_wrap_prepare_post("localhost:"DEFAULT_DBPORT"/query",NULL, 1, post_data);
	curl_wrap_perform(request, &result, &result_size);

	if(curl_wrap_response_code_get(request) != 200) {
		AFB_ERROR("Can't create database.");
		ret = -1;
	}

	curl_easy_cleanup(request);
	free(post_data);

	if(ret == 0)
		AFB_NOTICE("Database 'agl-collector' created");

	return ret;
}

void curl_cb(void *closure, int status, CURL *curl, const char *result, size_t size)
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

int unpack_metric(json_object *m, const char **name, const char **source, const char **unit, const char **identity, json_object **jv, uint64_t *timestamp)
{
	if (wrap_json_unpack(m, "{ss,s?s,s?s,s?s,so,sI!}",
					"name", name,
					"source", source,
					"unit", unit,
					"identity", identity,
					"value", jv,
					"timestamp", timestamp))
		return -1;
	else if (!json_object_is_type(*jv, json_type_boolean) &&
			 !json_object_is_type(*jv, json_type_double) &&
			 !json_object_is_type(*jv, json_type_int) &&
			 !json_object_is_type(*jv, json_type_string))
		return -1;

	return 0;
}

void concatenate(char* dest, const char* source, const char *sep)
{
	strncat(dest, sep, strlen(sep));
	strncat(dest, source, strlen(source));
}

char *make_query(char *query, const char *name, const char *source, const char *unit, const char *identity, json_object *jv, uint64_t timestamp)
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

	return query;
}

CURL *make_curl_write_post(const char *url, struct json_object *metric)
{
	CURL *curl;
	const char *name = NULL,
			*source = NULL,
			*unit = NULL,
			*identity = NULL;

	char *post_data[2], query[URL_MAXIMUM_LENGTH];
	bzero(query, URL_MAXIMUM_LENGTH);

	json_object *jv = NULL;
	uint64_t timestamp = 0;

	if(unpack_metric(metric, &name, &source, &unit, &identity, &jv, &timestamp)) {
		AFB_ERROR("ERROR unpacking metric. %s", json_object_to_json_string(metric));
		curl = NULL;
	}
	else {
		make_query(query, name, source, unit, identity, jv, timestamp);
		post_data[0] = query;
		post_data[1] = NULL;
		curl = curl_wrap_prepare_post(url, NULL, 1, (const char * const*)post_data);
	}

	return curl;
}

int do_write(struct afb_req req, const char* host, int port, json_object *metric)
{
	CURL *curl_request;
	char url[URL_MAXIMUM_LENGTH]; // Safe limit for most popular web browser
	memset(url, 0, sizeof(url));

	// Handle default host and port
	host = host ? host : DEFAULT_DBHOST;
	port = port > 0 ? port : atoi(DEFAULT_DBPORT);

	strncat(url, host, strlen(host));
	strncat(url, ":"DEFAULT_DBPORT"/write?db="DEFAULT_DB, strlen(":"DEFAULT_DBPORT"/write?db="DEFAULT_DB));
	curl_request = make_curl_write_post(url, metric);
	curl_wrap_do(curl_request, curl_cb, &req);

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
	int err = 0;
	err = curl_global_init(CURL_GLOBAL_DEFAULT);
	if (!err)
		err = dbping();
	else {
		AFB_ERROR("Something went wrong initiliazing libcurl. Abort");
	}

	return err;
}
