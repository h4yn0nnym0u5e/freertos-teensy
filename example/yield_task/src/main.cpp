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
 * @file    main.cpp
 * @brief   FreeRTOS example with custom yield task for Teensy boards
 * @author  Timo Sandmann
 * @date    20.12.2025
 */

#include "arduino_freertos.h"

#include "freertos_runtime_stats.h"

#include <atomic>


static void task1(void*) {
    pinMode(arduino::LED_BUILTIN, arduino::OUTPUT);
    while (true) {
        digitalWriteFast(arduino::LED_BUILTIN, arduino::LOW);
        vTaskDelay(pdMS_TO_TICKS(100));

        digitalWriteFast(arduino::LED_BUILTIN, arduino::HIGH);
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    vTaskDelete(nullptr);
}

static void task2(void*) {
    FreeRTOSRuntimeStats freertos_stats {};

    Serial.begin(0);
    while (true) {
        freertos_stats.print();
        vTaskDelay(pdMS_TO_TICKS(2'000));
    }

    vTaskDelete(nullptr);
}

namespace freertos {
extern TaskHandle_t g_yield_task;

EventResponder test_er {}; // example EventResponder that is triggered if a serial event occurs

#if configUSE_CUSTOM_YIELD_HANDLER == 1
static std::atomic<bool> yield_running {};

FLASHMEM void setup_yield() {
    ::xTaskCreate(
        [](void*) {
            while (true) {
                uint32_t yield_events {};
                if (::xTaskNotifyWait(~0ul, ~0ul, &yield_events, configYIELD_TASK_FREQUENCY_TICKS) != pdPASS) {
                    yield_events = yield_active_check_flags;
                }

                freertos::yield_running.store(true, std::memory_order_relaxed);

                if (yield_events & YIELD_CHECK_USB_SERIAL) {
                    if (Serial.available()) {
                        serialEvent();
                    }
                }
#if defined(USB_DUAL_SERIAL) || defined(USB_TRIPLE_SERIAL)
                if (yield_events & YIELD_CHECK_USB_SERIALUSB1) {
                    if (SerialUSB1.available()) {
                        serialEventUSB1();
                    }
                }
#endif
#ifdef USB_TRIPLE_SERIAL
                if (yield_events & YIELD_CHECK_USB_SERIALUSB2) {
                    if (SerialUSB2.available()) {
                        serialEventUSB2();
                    }
                }
#endif

                if (yield_events & YIELD_CHECK_HARDWARE_SERIAL) {
                    HardwareSerialIMXRT::processSerialEventsList();
                }

                if (yield_events & YIELD_CHECK_EVENT_RESPONDER) {
                    EventResponder::runFromYield();
                }

                ::xTaskNotifyStateClear(g_yield_task);

                freertos::yield_running.store(false, std::memory_order_relaxed);
            }
        },
        PSTR("YIELD"), configYIELD_TASK_STACK_SIZE, nullptr, 1, &g_yield_task);

    /* example code to test serialEvent and EventResponder from yield() */
    test_er.attach([](EventResponder& er) { Serial.printf("serial data received: 0x%x\r\n", er.getStatus()); });
}
#endif // configUSE_CUSTOM_YIELD_HANDLER == 1
} // namespace freertos

#if configUSE_CUSTOM_YIELD_HANDLER == 1
void yield() {
    const auto check_flags { yield_active_check_flags };
    if (!check_flags) {
        return; // nothing to do
    }

    bool expected {};
    if (!freertos::yield_running.compare_exchange_strong(expected, true, std::memory_order_relaxed)) {
        return;
    }

    uint32_t yield_needed {};
    if (check_flags & YIELD_CHECK_USB_SERIAL) {
        if (Serial.available()) {
            yield_needed |= YIELD_CHECK_USB_SERIAL;
        }
    }

#if defined(USB_DUAL_SERIAL) || defined(USB_TRIPLE_SERIAL)
    if (check_flags & YIELD_CHECK_USB_SERIALUSB1) {
        if (SerialUSB1.available()) {
            yield_needed |= YIELD_CHECK_USB_SERIALUSB1;
        }
    }
#endif
#ifdef USB_TRIPLE_SERIAL
    if (check_flags & YIELD_CHECK_USB_SERIALUSB2) {
        if (SerialUSB2.available()) {
            yield_needed |= YIELD_CHECK_USB_SERIALUSB2;
        }
    }
#endif

    if (check_flags & YIELD_CHECK_HARDWARE_SERIAL) {
        yield_needed |= YIELD_CHECK_HARDWARE_SERIAL;
    }

    if (check_flags & YIELD_CHECK_EVENT_RESPONDER) {
        yield_needed |= YIELD_CHECK_EVENT_RESPONDER;
    }

    if (yield_needed && freertos::g_yield_task) {
        ::xTaskNotify(freertos::g_yield_task, yield_needed, eSetBits);
    }

    freertos::yield_running.store(false, std::memory_order_relaxed);
}
#endif // configUSE_CUSTOM_YIELD_HANDLER == 1

/* example code to test serialEvent and EventResponder from yield() */
void serialEvent() {
    freertos::test_er.triggerEvent(Serial.read());
}

void setup() {
    Serial.begin(0);
    if (CrashReport) {
        Serial.print(CrashReport);
        Serial.println();
        Serial.flush();
    }

    Serial.println(PSTR("\r\nBooting FreeRTOS kernel " tskKERNEL_VERSION_NUMBER ". Built by gcc " __VERSION__ " (newlib " _NEWLIB_VERSION ") on " __DATE__ ". ***\r\n"));

    xTaskCreate(task1, "task1", 128, nullptr, 2, nullptr);
    xTaskCreate(task2, "task2", 512, nullptr, 2, nullptr);

    Serial.println(PSTR("setup(): starting scheduler..."));
    Serial.flush();

#if configUSE_CUSTOM_YIELD_HANDLER == 0
    freertos::test_er.attach([](EventResponder& er) { Serial.printf("serial data received: 0x%x\r\n", er.getStatus()); });
#endif // configUSE_CUSTOM_YIELD_HANDLER == 0

    vTaskStartScheduler();
}

void loop() {}
