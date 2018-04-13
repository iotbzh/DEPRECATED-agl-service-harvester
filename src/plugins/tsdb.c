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

#include <time.h>
#include "tsdb.h"

int influxdb_ping()
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

int db_ping()
{
	int ret = 0;
	if(influxdb_ping() == 0) ret = INFLUX;

	return ret;
}

u_int64_t get_ts()
{
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);

	return (int64_t)(ts.tv_sec) * (int64_t)1000000000 + (int64_t)(ts.tv_nsec);
}
