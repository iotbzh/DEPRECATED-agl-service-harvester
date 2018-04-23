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
#include <string.h>
#include <stdio.h>

#include "influxdb.h"

static size_t format_write_args(char *query, struct series_t *serie)
{
	char *ts;
	struct list *tags = serie->serie_columns.tags;
	struct list *fields = serie->serie_columns.fields;

	strncat(query, serie->name, strlen(serie->name));
	if(tags) {
		while(tags != NULL) {
			concatenate(query, tags->key, ",");
			if(json_object_is_type(tags->value, json_type_string))
				concatenate(query, json_object_get_string(tags->value), "=");
			else
				concatenate(query, json_object_to_json_string(tags->value), "=");
			tags = tags->next;
		}
	}
	if(fields) {
		int i = 0;
		for(struct list *it = fields; it != NULL; it = it->next) {
			if(!i)
				concatenate(query, fields->key, " ");
			else
				concatenate(query, fields->key, ",");
			if(json_object_is_type(fields->value, json_type_string))
				concatenate(query, json_object_get_string(fields->value), "=");
			else
				concatenate(query, json_object_to_json_string(fields->value), "=");
			i++;
		}
	}

	asprintf(&ts, "%lu", serie->timestamp);
	concatenate(query, ts, " ");
	strcat(query, "\n");

	return strlen(query);
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

CURL *make_curl_write_post(const char *url, json_object *metricsJ)
{
	CURL *curl = NULL;
	size_t lpd = 0, i = 0;
	char **post_data;
	char write[URL_MAXIMUM_LENGTH];
	struct series_t *serie = NULL;
	json_object *metricsArrayJ = NULL;

	if(json_object_is_type(metricsJ, json_type_array)) {
		lpd = json_object_array_length(metricsJ);
		metricsArrayJ = metricsJ;
	}
	else {
		metricsArrayJ = json_object_new_array();
		json_object_array_add(metricsArrayJ, metricsJ);
		lpd = 1;
	}

	serie = malloc(sizeof(struct series_t));
	post_data = malloc(lpd);

	for(i = 0; i < lpd; i++) {
		bzero(serie, sizeof(struct series_t));

		if(unpack_metric_from_api(json_object_array_get_idx(metricsArrayJ, i), serie)) {
			AFB_ERROR("ERROR unpacking metric. %s", json_object_to_json_string(metricsArrayJ));
			break;
		}
		else {
			bzero(write, URL_MAXIMUM_LENGTH);
			if(! serie->name) {
				post_data[i] = NULL;
			}
			else {
				format_write_args(write, serie);
				strcpy(post_data[i], write);
			}
		}
	}
	post_data[i] = NULL;

	/* Check that we just do not broke the for loop before trying preparing CURL
	   request object */
	curl = i == lpd ?
		   curl_wrap_prepare_post_binary(url, NULL, " ", (const char * const*)post_data) : NULL;
	free(serie);
	free(post_data);

	return curl;
}

CURL *influxdb_write(const char* host, const char *port, json_object *metricJ)
{
	char url[URL_MAXIMUM_LENGTH]; /* Safe limit for most popular web browser */
	make_url(url, sizeof(url), host, port, "write");
	return make_curl_write_post(url, metricJ);
}
