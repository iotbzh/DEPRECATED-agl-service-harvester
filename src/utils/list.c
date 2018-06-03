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

#include "list.h"
#include "string.h"
#include <stdio.h>
#include <stdlib.h>

void destroy_list(struct list* l)
{
    struct list* save;
    while (l != NULL) {
        save = l->next;
        free(l);
        l = save;
    }
}

void add_elt(struct list** l, const char* key, json_object* value)
{
    struct list* new_elt;

    // search tail
    new_elt = *l;
    while (new_elt != NULL) {
        l = &new_elt->next;
        new_elt = new_elt->next;
    }

    // alloc new elem
    new_elt = malloc(sizeof(struct list));
    if (!new_elt) {
        fprintf(stderr, "OUT of memory");
        exit(1);
    }

    *l = new_elt;
    new_elt->key = key;
    new_elt->value = value;
    new_elt->next = NULL;
}

void add_key(struct list** l, const char* key)
{
    add_elt(l, key, NULL);
}

int set_value(struct list* l, json_object* val, int index)
{
    int i;

    for (i = 0; i < index; i++) {
        l = l->next;
        if (l == NULL)
            return -1;
    }

    l->value = val;
    return 0;
}

struct list* get_elt(struct list* l, int index)
{
    int i;

    for (i = 0; i < index; i++) {
        l = l->next;
        if (l == NULL)
            return NULL;
    }

    return l;
}

struct list* find_elt_from_key(struct list* l, const char* key)
{
    while (l != NULL) {
        if (strcasecmp(l->key, key) == 0)
            return l;
        l = l->next;
    }
    return NULL;
}

json_object* find_key_value(struct list* l, const char* key)
{
    while (l != NULL) {
        if (strcasecmp(l->key, key) == 0)
            return l->value;
        l = l->next;
    }
    return NULL;
}
