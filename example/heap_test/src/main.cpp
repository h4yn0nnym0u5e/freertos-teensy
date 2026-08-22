/*
 * This file is part of the FreeRTOS port to Teensy boards.
 * Copyright (c) 2020-2026 Timo Sandmann
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
 * @file    main.cpp
 * @brief   FreeRTOS example for Teensy boards
 * @author  Timo Sandmann
 * @date    21.08.2026
 */

#include "arduino_freertos.h"

#include <cstring>


static void task1(void*) {
    pinMode(arduino::LED_BUILTIN, arduino::OUTPUT);
    while (true) {
        digitalWriteFast(arduino::LED_BUILTIN, arduino::LOW);
        vTaskDelay(pdMS_TO_TICKS(500));

        digitalWriteFast(arduino::LED_BUILTIN, arduino::HIGH);
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    vTaskDelete(nullptr);
}


extern uint8_t* _g_heap_start;
extern uint8_t* _g_heap_max;
extern uint8_t* _g_current_heap_end;
extern unsigned long _heap_start;
extern unsigned long _heap_end;
extern unsigned long _estack;
extern unsigned long _ebss;


static void task2(void*) {
    static constexpr uint32_t ALLOCATION_STEP { 256 };
    static constexpr uint32_t MAX_ALLOCATIONS { 60 };

    uint32_t i {};
    static void* allocations[MAX_ALLOCATIONS] {};

    while (true) {
        freertos::print_ram_usage();

        if (i < MAX_ALLOCATIONS) {
            allocations[i] = malloc(i * ALLOCATION_STEP);
            Serial.printf("malloc(%u) = %p\r\n", i * ALLOCATION_STEP, allocations[i]);

            ++i;
            uint32_t alloc_sum {};
            for (uint32_t j {}; j < i; ++j) {
                alloc_sum += j * ALLOCATION_STEP;
            }
            Serial.printf("allocated so far: %u bytes (%u KB)\r\n", alloc_sum, alloc_sum / 1'024);
        } else if (i < MAX_ALLOCATIONS * 2) {
            free(allocations[i - MAX_ALLOCATIONS]);
            Serial.printf("free(%p)\r\n", allocations[i - MAX_ALLOCATIONS]);

            ++i;
            uint32_t free_sum {};
            for (uint32_t j {}; j < i - MAX_ALLOCATIONS; ++j) {
                free_sum += j * ALLOCATION_STEP;
            }
            Serial.printf("freed so far: %u bytes (%u KB)\r\n", free_sum, free_sum / 1'024);
        } else {
            i = 0;
        }

        vTaskDelay(pdMS_TO_TICKS(1'000));
    }

    vTaskDelete(nullptr);
}

void setup() {
    Serial.begin(0);
    if (CrashReport) {
        Serial.print(CrashReport);
        Serial.println();
        Serial.flush();
    }

    Serial.println(
        PSTR("\r\nBooting FreeRTOS kernel " tskKERNEL_VERSION_NUMBER ". Built by gcc " __VERSION__ " (newlib " _NEWLIB_VERSION ") on " __DATE__ ". ***\r\n"));
    Serial.printf(PSTR("_g_heap_start=0x%x, _g_heap_max=0x%x, _g_current_heap_end=0x%x\r\n"), reinterpret_cast<uintptr_t>(_g_heap_start),
        reinterpret_cast<uintptr_t>(_g_heap_max), reinterpret_cast<uintptr_t>(_g_current_heap_end));
    Serial.printf(PSTR("_heap_start=0x%x, _heap_end=0x%x\r\n"), reinterpret_cast<uintptr_t>(&_heap_start), reinterpret_cast<uintptr_t>(&_heap_end));

    if (_g_heap_start >= reinterpret_cast<uint8_t*>(&_ebss) && _g_heap_start <= reinterpret_cast<uint8_t*>(&_estack)) {
        Serial.println(PSTR("Heap is placed in DTCM"));
    } else if (_g_heap_start >= reinterpret_cast<uint8_t*>(&_heap_start) && _g_heap_start <= reinterpret_cast<uint8_t*>(_heap_end)) {
        Serial.println(PSTR("Heap is placed in RAM"));
    } else {
        Serial.println(PSTR("Invalid heap placement"));
    }
    Serial.println();

    xTaskCreate(task1, "task1", 128, nullptr, 2, nullptr);
    xTaskCreate(task2, "task2", 512, nullptr, 2, nullptr);

    Serial.println(PSTR("setup(): starting scheduler..."));
    Serial.flush();

    vTaskStartScheduler();
}

void loop() {}
