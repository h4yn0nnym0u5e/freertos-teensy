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
 * @brief   FreeRTOS example for Teensy boards
 * @author  Timo Sandmann
 * @date    17.05.2020
 */

#include "arduino_freertos.h"
#include "yieldStuff.h"

#define noCPU_HOG

uint32_t dt1 = 250, dt2 = 513, dt3 = 5'000; // opportunity to do nasty stuff...

#define TC 6
struct taskInfo {
  TaskHandle_t handle;
  TaskStatus_t status;  
} taskStatuses[TC];

/*
 * List some interesting info about the running tasks
 */
void listTasksInfo(void)
{
  taskStatuses[0].handle = xTaskGetIdleTaskHandle();
  if (!taskStatuses[4].handle) // no custom yield task handle, but...
    taskStatuses[5].handle = freertos::g_yield_task; // ...maybe there's a library one
  
  for (int i=0;i<TC;i++)
    if (taskStatuses[i].handle) 
      vTaskGetInfo(taskStatuses[i].handle, &taskStatuses[i].status, pdFALSE, eInvalid);
      
  for (int i=0;i<TC;i++)
    if (taskStatuses[i].handle) 
      Serial.printf("%-10s ",taskStatuses[i].status.pcTaskName);
  Serial.println();
        
  for (int i=0;i<TC;i++)
    if (taskStatuses[i].handle) 
      Serial.printf("%-10lu ",taskStatuses[i].status.ulRunTimeCounter);
  Serial.println();      

  Serial.println();      
}


void taskStart(uint32_t d, const char* extra="")
{
  TaskStatus_t TaskDetails; 

  vTaskGetInfo(NULL,&TaskDetails,pdFALSE,eInvalid);
  Serial.printf("Started task '%s' at priority %u with delay %d%s\n", 
                TaskDetails.pcTaskName, 
                TaskDetails.uxCurrentPriority,
                d,
                extra);
}

static void task1(void* pvParams) {
    uint32_t& delVal = *(uint32_t*) pvParams;
    taskStart(delVal);
    
    pinMode(arduino::LED_BUILTIN, arduino::OUTPUT);
    while (true) {
        digitalWriteFast(arduino::LED_BUILTIN, arduino::LOW);
        vTaskDelay(pdMS_TO_TICKS(delVal));

        digitalWriteFast(arduino::LED_BUILTIN, arduino::HIGH);
        vTaskDelay(pdMS_TO_TICKS(delVal));
    }

    vTaskDelete(nullptr);
}


void waitForMs(uint32_t ms)
{
#if defined(CPU_HOG)
  elapsedMillis t = 0;
  while (t < ms) // hog CPU time
    ;  
#else
  vTaskDelay(pdMS_TO_TICKS(ms)); // share nicely
#endif // defined(CPU_HOG)
      
}

static void task2(void* pvParams) {
    uint32_t& delVal = *(uint32_t*) pvParams;
#if defined(CPU_HOG)
    taskStart(delVal,"; hogging CPU");
#else    
    taskStart(delVal);
#endif // defined(CPU_HOG)
    Serial.begin(0);
    while (true) {
        Serial.print("TICK ");
        dt1 = 50;
        waitForMs(delVal-1); // may hog CPU, or not
        
        Serial.print("TOCK ");
        dt1 = 100;
        waitForMs(delVal-1); // may hog CPU, or not

        // yield once per loop: this is explicit, 
        // bypassing the yield task if one exists
        yield_direct();
    }

    vTaskDelete(nullptr);
}

static void task3(void* pvParams) {
    uint32_t& delVal = *(uint32_t*) pvParams;
    taskStart(delVal);

    uint32_t started = xTaskGetTickCount();
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(delVal));
        uint32_t elapsed = xTaskGetTickCount() - started;

        Serial.print("\n@ ");
        Serial.print(elapsed);
        Serial.print(": idle count: ");
        Serial.print(idle_count);
        Serial.print("; yield count: ");
        Serial.print(yield_count);
        Serial.print("; yield task count: ");
        Serial.print(yield_task_count);
        Serial.print("; yield duration: ");
        Serial.print(measureYieldDuration()); // calls yield()
        Serial.println("µs");

        listTasksInfo();
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

    Serial.println(PSTR("\r\nBooting FreeRTOS kernel " tskKERNEL_VERSION_NUMBER ". Built by gcc " __VERSION__ " (newlib " _NEWLIB_VERSION ") on " __DATE__ ". ***\r\n"));

    //static uint32_t dt1 = 250, dt2 = 500, dt3 = 4'875; // address goes invalid without 'static'
    // N.B. printf is expensive on stack use!
    //          fn      name   stack  params pri ^handle
    xTaskCreate(task1, "blink", 1024,  &dt1,  2, &taskStatuses[1].handle);
    xTaskCreate(task2, "ticky", 1024,  &dt2,  1, &taskStatuses[2].handle);
    xTaskCreate(task3, "newln", 1024,  &dt3,  2, &taskStatuses[3].handle);

    initYieldEvent();
    taskStatuses[4].handle = initCustomYield();
    Serial.printf("%ssing custom yield()\n",taskStatuses[4].handle?"U":"Not u");

    Serial.println(PSTR("setup(): starting scheduler..."));
    Serial.flush();

    vTaskStartScheduler();
}

void loop() {}
