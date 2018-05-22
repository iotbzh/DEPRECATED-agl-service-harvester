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

void influxdb_write_curl_cb(void *closure, int status, CURL *curl, const char *result, size_t size)
{
	AFB_ReqT request = (AFB_ReqT)closure;
	long rep_code = curl_wrap_response_code_get(curl);
	switch(rep_code) {
		case 204:
			AFB_ReqDebug(request, "Request correctly written");
			AFB_ReqSucess(request, NULL, "Request has been successfully written");
			break;
		case 400:
			AFB_ReqFail(request, "Bad request", result);
			break;
		case 401:
			AFB_ReqFail(request, "Unauthorized access", result);
			break;
		case 404:
			AFB_ReqFail(request, "Not found", result);
			AFB_ReqNotice(request, "Attempt to create the DB '"DEFAULT_DB"'");
			create_database(request);
			break;
		case 500:
			AFB_ReqFailF(request, "Timeout", "Overloaded server: %s", result);
			break;
		default:
			AFB_ReqFail(request, "Failure", "Unexpected behavior.");
			break;
	}
}

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

	return strlen(query);
}

CURL *make_curl_write_post(AFB_ApiT apiHandle, const char *url, json_object *metricsJ)
{
	CURL *curl = NULL;
	size_t lpd = 0, len_write = 0, i = 0;
	char **post_data;
	char write[URL_MAXIMUM_LENGTH] = "";
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
	post_data = calloc(lpd + 1, sizeof(void*));

	for(i = 0; i < lpd; i++) {
		memset(serie, 0, sizeof(struct series_t));

		if(unpack_metric_from_api(json_object_array_get_idx(metricsArrayJ, i), serie)) {
			AFB_ApiError(apiHandle, "ERROR unpacking metric. %s", json_object_to_json_string(metricsArrayJ));
			break;
		}
		else {
			if(! serie->name) {
				post_data[i] = NULL;
			}
			else {
				len_write = format_write_args(write, serie);
				if(len_write) {
					post_data[i] = malloc(len_write + 1);
					strcpy(post_data[i], write);
					memset(write, 0, len_write);
				}
			}
		}
	}

	/* Check that we just do not broke the for loop before trying preparing CURL
	   request object */
	curl = i == lpd ?
		   curl_wrap_prepare_post_unescaped(url, NULL, "\n", (const char * const*)post_data) : NULL;
	free(serie);
	for(i = 0; i < lpd; i++)
		free(post_data[i]);
	free(post_data);

	return curl;
}

CURL *influxdb_write(AFB_ApiT apiHandle, const char* host, const char *port, json_object *metricJ)
{
	char url[URL_MAXIMUM_LENGTH]; /* Safe limit for most popular web browser */
	make_url(url, sizeof(url), host, port, "write");
	return make_curl_write_post(apiHandle, url, metricJ);
}

CTLP_CAPI(write_to_influxdb, source, argsJ, eventJ)
{
	AFB_ReqT request = source->request;
	const char *port = NULL;
	const char *host = NULL;
	CURL *curl_request;

	json_object *req_args = AFB_ReqJson(request),
				*portJ = NULL,
				*metric = NULL;

	if(wrap_json_unpack(req_args, "{s?s,s?o,so!}",
			"host", &host,
			"port", &portJ,
			"metric", &metric) || ! metric)
		AFB_ReqFail(request, "Failed", "Error processing arguments. Miss metric\
JSON object or malformed");
	else
		port = json_object_is_type(portJ, json_type_null) ?
			NULL : json_object_to_json_string(portJ);

	curl_request = influxdb_write(source->api, host, port, metric);
	curl_wrap_do(curl_request, influxdb_write_curl_cb, request);

	return 0;
}
