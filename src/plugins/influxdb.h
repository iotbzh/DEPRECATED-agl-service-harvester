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

#include "wrap-json.h"
#include "tsdb.h"

int create_database();

int unpack_metric_from_binding(json_object *m, const char **name, const char **source, const char **unit, const char **identity, json_object **jv, uint64_t *timestamp);

void concatenate(char* dest, const char* source, const char *sep);

size_t make_url(char *url, size_t l_url, const char *host, const char *port, const char *endpoint);
