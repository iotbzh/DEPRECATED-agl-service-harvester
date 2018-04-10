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

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <systemd/sd-event.h>

#include "influxdb.h"

CURL *make_curl_query_get(const char *url)
{
	CURL *curl;
	char *args[5];
	char *last_ts[30];

	int fd_last_read = afb_daemon_rootdir_open_locale("last_db_read", O_CREAT, NULL);
	if (fd_last_read < 0)
		return NULL;

	read(fd_last_read, last_ts, sizeof(last_ts));

	args[0] = "epoch";
	args[1] = "ns";
	args[2] = "q";
	args[3] = "SELECT * FROM /^.*$/";
	args[4] = NULL;

	curl = curl_wrap_prepare_get(url, NULL, (const char * const*)args);

	return curl;
}

int unpack_metric_from_db(json_object *metric)
{
	const char *name;
	json_object *columns = NULL, *values = NULL;

	wrap_json_unpack(metric, "{ss, so, so!}",
					 "name", &name,
					 "columns", &columns,
					 "values", &values);
}

int unpack_series(json_object *series)
{
	size_t length_series = json_object_array_length(series);

}

void forward_to_garner(const char *result, size_t size)
{
	int id = 0;

	json_object *series = NULL,
				*db_dump = json_tokener_parse(result);
	wrap_json_unpack(db_dump, "{s[{si,so}]}",
					 "id", &id,
					 "series", &series);

	unpack_series(series);

}

void influxdb_read_curl_cb(void *closure, int status, CURL *curl, const char *result, size_t size)
{
	long rep_code = curl_wrap_response_code_get(curl);
	switch(rep_code) {
		case 204:
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

int influxdb_read(sd_event_source *s, uint64_t usec, void *userdata)
{
	CURL *curl;
	struct reader_args *a = NULL;
	if(userdata)
		a = (struct reader_args*)userdata;
	else
		return -1;

	char url[URL_MAXIMUM_LENGTH]; /* Safe limit for most popular web browser */

	make_url(url, sizeof(url), a->host, a->port, "query");
	curl = make_curl_query_get(url);
	curl_wrap_do(curl, influxdb_read_curl_cb, NULL);

	return 0;
}

int influxdb_reader(void *args)
{
	uint64_t usec;
	struct sd_event_source *evtSource;

	/* Set a cyclic cb call each 1s to call the read callback */
	sd_event_now(afb_daemon_get_event_loop(), CLOCK_MONOTONIC, &usec);
	return sd_event_add_time(afb_daemon_get_event_loop(), &evtSource, CLOCK_MONOTONIC, usec+1000000, 250, influxdb_read, args);
}
