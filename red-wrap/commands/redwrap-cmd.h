/*
 * Copyright (C) 2022 "IoT.bzh"
 *
 * Author: Valentin Lefebvre <valentin.lefebvre@iot.bzh
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
 */

#pragma once

//////////////////////////////////////////////////////////////////////////////
//                             INCLUDES                                     //
//////////////////////////////////////////////////////////////////////////////

// --- Redwrap includes
#include "redwrap-main.h"

// --- Standard includes
#include <string.h>
#include <errno.h>

//////////////////////////////////////////////////////////////////////////////
//                             FUNCTIONS                                    //
//////////////////////////////////////////////////////////////////////////////

/**
 * @brief execute redwrap-dnf command according arguments
 * 
 * @param[in] argc Number of arguments
 * @param[in] argv Array of arguments
 * @return 0 in success negative otherwise
 */
int redwrap_dnf_cmd_exec(int argc, char *argv[]);

/**
 * @brief execute redwrap command according arguments
 * 
 * @param argc Number of arguments
 * @param argv Array of arguments
 * @return 0 in success negative otherwise
 */
int redwrap_cmd_exec(int argc, char *argv[]);
