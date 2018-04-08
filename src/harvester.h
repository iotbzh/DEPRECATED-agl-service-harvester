/*
 * Copyright (C) 2015, 2016 "IoT.bzh"
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

#define AFB_BINDING_VERSION 2
#include <afb/afb-binding.h>

union port {
	int i_port;
	char c_port[5]; // Available ports 1-65535
};

union metric_value {
	enum metric_type {b = 0, i, d, str} type;
	int b_value;
	int i_value;
	double d_value;
	char *str_value;
};

int init();
