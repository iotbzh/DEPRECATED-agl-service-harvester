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
#ifndef _LIST_H_
#define _LIST_H_

#include <stdlib.h>
#include <json-c/json.h>

struct list {
	const char *key;
	json_object *value;
	struct list *next;
};

void destroy_list(struct list *l);
void add_elt(struct list **l, const char *key, json_object *value);
void add_key(struct list **l, const char *key);
int set_value(struct list *l, json_object *val, int index);
struct list *get_elt(struct list *l, int index);
struct list *find_elt_from_key(struct list *l, const char *key);
json_object *find_key_value(struct list *l, const char *key);

#endif
