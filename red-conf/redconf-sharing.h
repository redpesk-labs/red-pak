/*
* Copyright (C) 2022-2025 IoT.bzh Company
* Author: José Bollo <jose.bollo@iot.bzh>
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*   http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
*/


#ifndef _RED_CONF_SHARING_INCLUDE_
#define _RED_CONF_SHARING_INCLUDE_

/** four-state value of sharing */
typedef enum {
    /** sharing is not set */
    RED_CONF_SHARING_UNSET = 0,
    /** sharing is set to disabled */
    RED_CONF_SHARING_DISABLED = 1,
    /** sharing is set to enabled */
    RED_CONF_SHARING_ENABLED = 2,
    /** sharing is set to a path */
    RED_CONF_SHARING_JOIN = 3,
}
    redConfSharingE;

/**
 * Gets the sharing type of the sharing value
 * @param value sharing value to check
 * @return the sharing type
 */
extern redConfSharingE sharing_type(const char *value);

/**
 * Gets the sharing string of the sharing value
 * @param value sharing value to get
 * @return the sharing string
 */
extern const char *sharing_string(const char *value);

#endif