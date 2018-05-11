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

#define _GNU_SOURCE
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "influxdb.h"
#include "tsdb.h"
#include "wrap-json.h"
#include "../utils/list.h"

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


int create_database()
{
	int ret = 0;
	char *result;
	size_t result_size;

	// Declare query to be posted
	const char *post_data[2];
	post_data[0] = "q=CREATE DATABASE \""DEFAULT_DB"\"";
	post_data[1] = NULL;

	CURL *request = curl_wrap_prepare_post_unescaped("localhost:"DEFAULT_DBPORT"/query",NULL, " ", post_data);
	curl_wrap_perform(request, &result, &result_size);

	if(curl_wrap_response_code_get(request) != 200) {
		AFB_ERROR("Can't create database.");
		ret = ERROR;
	}

	curl_easy_cleanup(request);

	if(ret == 0)
		AFB_NOTICE("Database '"DEFAULT_DB"' created");

	return ret;
}

void unpack_values(void *l, json_object *valuesJ, const char *key)
{
	struct list **oneList = (struct list **)l;

	/* Append a suffix to be able to differentiate tags and fields at reading
	   time */
	char *suffixed_key = calloc(1, strlen(key) + 3);
	strcpy(suffixed_key, key);
	strcat(suffixed_key, "_f");

	add_elt(oneList, suffixed_key, valuesJ);
}

void unpack_metadata(void *l, json_object *valuesJ, const char *key)
{
	struct list **oneList = (struct list **)l;

	/* Append a suffix to be able to differentiate tags and fields at reading
	   time */
	char *suffixed_key = calloc(1, strlen(key) +3);
	strcat(suffixed_key, key);
	strcat(suffixed_key, "_t");

	add_elt(oneList, suffixed_key, valuesJ);
}

void unpacking_from_api(void *s, json_object *valueJ, const char *key)
{
	size_t key_length = strlen(key);
	struct series_t *serie = (struct series_t*)s;

	/* Treat the 2 static key that could have been specified */
	if(strcasecmp("name", key) == 0)
		serie->name = json_object_get_string(valueJ);
	else if(strcasecmp("timestamp", key) == 0)
		serie->timestamp = get_ts();
	else if(strcasecmp("metadata", key) == 0)
		wrap_json_object_for_all(valueJ, unpack_metadata, (void*)&serie->serie_columns.tags);
	else if(strcasecmp("value", key) == 0 || strcasecmp("values", key) == 0)
		wrap_json_object_for_all(valueJ, unpack_values, (void*)&serie->serie_columns.fields);
	/* Treat all key looking for tag and field object. Those ones could be find
	   with the last 2 character. '_t' for tag and '_f' that are the keys that
	   could be indefinite. Cf influxdb documentation:
	   https://docs.influxdata.com/influxdb/v1.5/write_protocols/line_protocol_reference/ */
	else if(strncasecmp(&key[key_length-2], "_t", 2) == 0)
		add_elt(&serie->serie_columns.tags, key, valueJ);
	else if(strncasecmp(&key[key_length-2], "_f", 2) == 0)
		add_elt(&serie->serie_columns.fields, key, valueJ);
}

int unpack_metric_from_api(json_object *m, struct series_t *serie)
{
	wrap_json_object_for_all(m, unpacking_from_api, serie);

	if(!serie->timestamp)
		serie->timestamp = get_ts();

	return 0;
}
