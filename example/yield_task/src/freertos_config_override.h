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
 * @file    freertos_config_override.h
 * @brief   FreeRTOS configuration overrides for Teensy boards
 * @author  Timo Sandmann
 * @date    20.12.2025
 */

#pragma once

#undef configUSE_CUSTOM_YIELD_HANDLER
#define configUSE_CUSTOM_YIELD_HANDLER 1
