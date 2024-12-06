/*
* Copyright (C) 2020 "IoT.bzh"
* author: José Bollo <jose.bollo@iot.bzh>
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

#include "redconf-sharing.h"

#include <string.h>

redConfSharingE sharing_type(const char *value)
{
    if (value == NULL)
        return RED_CONF_SHARING_UNSET;
    if (strcasecmp(value, "enabled") == 0)
        return RED_CONF_SHARING_ENABLED;
    if (strcasecmp(value, "disabled") == 0)
        return RED_CONF_SHARING_DISABLED;
    if (strcasecmp(value, "unset") == 0)
        return RED_CONF_SHARING_UNSET;
    return RED_CONF_SHARING_JOIN;
}

const char *sharing_string(const char *value)
{
    switch (sharing_type(value))
    {
    default:
    case RED_CONF_SHARING_UNSET:
        return "Unset";
    case RED_CONF_SHARING_ENABLED:
        return "Enabled";
    case RED_CONF_SHARING_DISABLED:
        return "Disabled";
    case RED_CONF_SHARING_JOIN:
        return value;
    }
}

