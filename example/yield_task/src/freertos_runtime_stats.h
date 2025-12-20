/*
 * This file is part of the FreeRTOS port to Teensy boards.
 * Copyright (c) 2020-2025 Timo Sandmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file    freertos_runtime_stats.h
 * @brief   Generate and print runtime stats on FreeRTOS
 * @author  Timo Sandmann
 * @date    30.07.2022
 */


#pragma once

#include "arduino_freertos.h"

#include <cstdint>
#include <vector>
#include <map>


class FreeRTOSRuntimeStats {
    std::vector<TaskStatus_t> task_data_;
    std::vector<std::pair<TaskHandle_t, float>> current_runtimes_;
    std::map<TaskHandle_t, uint32_t> last_runtimes_;
    uint32_t last_total_runtime_;

public:
    FreeRTOSRuntimeStats();

    bool generate();
    bool print();
};
