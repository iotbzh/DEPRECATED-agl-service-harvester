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

#include "influxdb.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <systemd/sd-event.h>

#include "../utils/list.h"

struct metrics_list {
	struct series_t serie;
	json_object *metricsJ;
};

static void fill_n_send_values(void *c, json_object *valuesJ)
{
	struct list *it = NULL;
	int length = json_object_get_string_len(valuesJ), i = 0, j = 0;
	struct metrics_list *m_list = (struct metrics_list *)c;
	json_object *one_metric = json_object_new_object();

	for (i = 0; i < length; i++) {
		if(!i)
			m_list->serie.timestamp = json_object_get_int64(valuesJ);
		else {
			if(set_value(m_list->serie.serie_columns.tags, valuesJ, i)) {
				if(set_value(m_list->serie.serie_columns.fields, valuesJ, j)) {
					AFB_ERROR("No tags nor fields fits.");
				}
				j++;
			}
		}
	}

	/* Build a metric object to add in the JSON array */
	json_object_object_add(one_metric, "name", json_object_new_string(m_list->serie.name));
	json_object_object_add(one_metric, "timestamp", json_object_new_int64(m_list->serie.timestamp));
	for(it = m_list->serie.serie_columns.tags; it != NULL; it = it->next)
		json_object_object_add(one_metric, m_list->serie.serie_columns.tags->key, m_list->serie.serie_columns.tags->value);
	for(it = m_list->serie.serie_columns.fields; it != NULL; it = it->next)
		json_object_object_add(one_metric, m_list->serie.serie_columns.fields->key, m_list->serie.serie_columns.fields->value);

	json_object_array_add(m_list->metricsJ, one_metric);
}

static void fill_key(void *c, json_object *columnJ)
{
	int length = json_object_get_string_len(columnJ);
	const char *column = json_object_get_string(columnJ);
	struct metrics_list *m_list = (struct metrics_list *)c;

	if(strncasecmp(&column[length-2], "_t", 2) == 0) {
		add_key(&m_list->serie.serie_columns.tags, column);
	}
	else if(strncasecmp(&column[length-2], "_f", 2) == 0) {
		add_key(&m_list->serie.serie_columns.fields, column);
	}
}

static void unpack_metric_from_db(void *ml, json_object *metricJ)
{
	struct metrics_list *m_list = (struct metrics_list*)ml;
	json_object *columnsJ = NULL, *valuesJ = NULL;

	if(wrap_json_unpack(metricJ, "{ss, so, so!}",
					 "name", &m_list->serie.name,
					 "columns", &columnsJ,
					 "values", &valuesJ)) {
		AFB_ERROR("Unpacking metric goes wrong");
		return;
	 }

	wrap_json_array_for_all(columnsJ, fill_key, m_list);
	wrap_json_array_for_all(valuesJ, fill_n_send_values, m_list);
}

static json_object *unpack_series(json_object *seriesJ)
{
	struct metrics_list m_list = {
		.serie = {
			.name = NULL,
			.serie_columns = {
				.tags = NULL,
				.fields = NULL
			},
			.timestamp = 0
		},
		.metricsJ = json_object_new_array()
	};

	wrap_json_array_for_all(seriesJ, unpack_metric_from_db, (void*)&m_list);

	return m_list.metricsJ;
}

static void forward_to_garner(const char *result, size_t size)
{
	int id = 0;
	json_object *resultsJ = NULL,
				*seriesJ = NULL,
				*metrics2send = NULL,
				*call_resultJ = NULL,
				*db_dumpJ = json_tokener_parse(result);

	if( wrap_json_unpack(db_dumpJ, "{so!}",
					 "results", &resultsJ) ||
		wrap_json_unpack(resultsJ, "[{si,so!}]",
					 "statement_id", &id,
					 "series", &seriesJ)) {
//		AFB_DEBUG("Unpacking results from influxdb request. Request results was:\n%s", result);
		return;
	}

	if(seriesJ) {
		metrics2send = unpack_series(seriesJ);
		if(json_object_array_length(metrics2send)) {
			if(afb_service_call_sync("garner", "write", metrics2send, &call_resultJ)) {
				AFB_ERROR("Metrics were sent but not done, an error happens. Details: %s", json_object_to_json_string(call_resultJ));
			}
		}
	}
	else {
		AFB_ERROR("Empty response. Request results was:\n%s", result);
	}
}

static void influxdb_read_curl_cb(void *closure, int status, CURL *curl, const char *result, size_t size)
{
	long rep_code = curl_wrap_response_code_get(curl);
	switch(rep_code) {
		case 200:
			AFB_DEBUG("Read correctly done");
			forward_to_garner(result, size);
			break;
		case 400:
			AFB_ERROR("Unacceptable request. %s", result);
			break;
		case 401:
			AFB_ERROR("Invalid authentication. %s", result);
			break;
		default:
			AFB_ERROR("Unexptected behavior. %s", result);
			break;
	}
}

static CURL *make_curl_query_get(const char *url)
{
	CURL *curl;
	char *args[5];
	char query[255] = {};
	char last_ts[30] = {};
	char *now;
	int length_now;

	args[0] = "epoch";
	args[1] = "ns";
	args[2] = "q";
	strcat(query, "SELECT * FROM /^.*$/");
	args[4] = NULL;
	length_now = asprintf(&now, "%lu", get_ts());

	int rootdir_fd = afb_daemon_rootdir_get_fd();
	int fd_last_read = openat(rootdir_fd, "last_db_read", O_CREAT | O_RDWR, S_IRWXU);
	if (fd_last_read < 0)
		return NULL;

	/* Reading last timestamp recorded and get metric from that point until now
	   else write the last timestamp */
	if(read(fd_last_read, last_ts, sizeof(last_ts)) == 0) {
		if(write(fd_last_read, now, length_now) != length_now)
			AFB_ERROR("Error writing last_db_read file: %s\n", strerror( errno ));
	}
	else {
		strcat(query, " WHERE time >= ");
		strncat(query, last_ts, strlen(last_ts));
		close(fd_last_read);
		fd_last_read = openat(rootdir_fd, "last_db_read", O_TRUNC | O_RDWR);
		if (write(fd_last_read, now, length_now) != length_now)
			AFB_ERROR("Error writing last_db_read file: %s", strerror( errno ));
	}

	args[3] = query;
	curl = curl_wrap_prepare_get(url, NULL, (const char * const*)args);

	return curl;
}

static int influxdb_read(sd_event_source *s, uint64_t usec, void *userdata)
{
	CURL *curl;
	struct reader_args *a = NULL;
	if(userdata)
		a = (struct reader_args*)userdata;
	else
		return ERROR;

	char url[URL_MAXIMUM_LENGTH]; /* Safe limit for most popular web browser */

	make_url(url, sizeof(url), a->host, a->port, "query");
	curl = make_curl_query_get(url);
	curl_wrap_do(curl, influxdb_read_curl_cb, NULL);

	/* Reschedule next run */
	sd_event_source_set_time(s, usec + a->delay);

	return 0;
}

int influxdb_reader(void *args)
{
	int err = 0;
	uint64_t usec;
	struct sd_event_source *evtSource = NULL;

	/* Set a cyclic cb call each 1s to call the read callback */
	sd_event_now(afb_daemon_get_event_loop(), CLOCK_MONOTONIC, &usec);
	err = sd_event_add_time(afb_daemon_get_event_loop(), &evtSource, CLOCK_MONOTONIC, usec+1000000, 250, influxdb_read, args);
	if(!err)
		err = sd_event_source_set_enabled(evtSource, SD_EVENT_ON);

	return err;
}
