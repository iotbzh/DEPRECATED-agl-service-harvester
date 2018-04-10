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
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "tsdb.h"
#include "wrap-json.h"

int create_database()
{
	int ret = 0;
	char *result;
	size_t result_size;

	// Declare query to be posted
	const char *post_data[2];
	post_data[0] = "q=CREATE DATABASE \""DEFAULT_DB"\"";
	post_data[1] = NULL;

	CURL *request = curl_wrap_prepare_post("localhost:"DEFAULT_DBPORT"/query",NULL, 1, post_data);
	curl_wrap_perform(request, &result, &result_size);

	if(curl_wrap_response_code_get(request) != 200) {
		AFB_ERROR("Can't create database.");
		ret = -1;
	}

	curl_easy_cleanup(request);

	if(ret == 0)
		AFB_NOTICE("Database 'agl-collector' created");

	return ret;
}

int unpack_metric_from_binding(json_object *m, const char **name, const char **source, const char **unit, const char **identity, json_object **jv, uint64_t *timestamp)
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

size_t make_url(char *url, size_t l_url, const char *host, const char *port, const char *endpoint)
{
	bzero(url, l_url);

	/* Handle default host and port */
	host = host ? host : DEFAULT_DBHOST;
	port = port ? port : DEFAULT_DBPORT;

	strncat(url, host, strlen(host));
	strncat(url, ":", 1);
	strncat(url, port, strlen(port));
	strncat(url, "/", 1);
	strncat(url, endpoint, strlen(endpoint));
	strncat(url, "?db="DEFAULT_DB, strlen("?db="DEFAULT_DB));

	return strlen(url);
}
