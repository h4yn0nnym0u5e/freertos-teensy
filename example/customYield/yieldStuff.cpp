#include <atomic>
#include "arduino_freertos.h"
#include <EventResponder.h>
#include "yieldStuff.h"

/******************************************************************/
/*
 * Define our own freertos::idle_hook(),
 * overriding the weak definition in the library.
 * 
 * We simply count up every time the idle
 * task runs, which could be infrequent as its
 * priority is 0. If it doesn't run then
 * yield() may never be run, either...
 */
volatile uint32_t idle_count;
namespace freertos 
{
  void idle_hook(void)
  {
    idle_count++;  
  }
}


/******************************************************************/
/*
 * Create an EventResponder which counts the number of
 * times it has run, and re-triggers itself. This should
 * give a good indication of the number of times
 * yield() has been called.
 */
volatile uint32_t yield_count;
static void yieldEventResponse(EventResponderRef ref)
{
  yield_count++; // count events
  ref.triggerEvent(); // re-trigger ourselves
}


void initYieldEvent(void)
{
  static EventResponder yieldResponder;
  yieldResponder.attach(yieldEventResponse); // respond in yield()
  yieldResponder.triggerEvent();             // first trigger
}


/******************************************************************/
/*
 * Create a "library" function which calls yield() once,
 * and returns the time taken in microseconds
 */
uint32_t measureYieldDuration(void)
{
  elapsedMicros em = 0;
  yield();
  return em;
}


/******************************************************************/
volatile uint32_t yield_task_count; // increments if yield task is running, counting loops
#if defined(configUSE_CUSTOM_YIELD_HANDLER) && configUSE_CUSTOM_YIELD_HANDLER != 0
/*
 * Custom yield implementation: 
 * reproduce the old behaviour, roughly
 */

namespace freertos {
  
TaskHandle_t g_yield_task {}; 

/*
 * Actual implementation of yield() code, 
 * copied into the 'freertos' namespace
 * 
 * Copied from library: it could be aliased
 * to allow direct re-use and ease maintenance
 */
FLASHMEM void xyield() {
    static std::atomic<bool> running { false };

    const auto check_flags { yield_active_check_flags };
    if (!check_flags) {
        return; // nothing to do
    }

    bool expected {};
    if (!running.compare_exchange_strong(expected, true, std::memory_order_relaxed)) {
        return;
    }

    // USB Serial - Add hack to minimize impact...
    if (check_flags & YIELD_CHECK_USB_SERIAL) {
        if (Serial.available()) {
            serialEvent();
        }
    }

#if defined(USB_DUAL_SERIAL) || defined(USB_TRIPLE_SERIAL)
    if (check_flags & YIELD_CHECK_USB_SERIALUSB1) {
        if (SerialUSB1.available()) {
            serialEventUSB1();
        }
    }
#endif
#ifdef USB_TRIPLE_SERIAL
    if (check_flags & YIELD_CHECK_USB_SERIALUSB2) {
        if (SerialUSB2.available()) {
            serialEventUSB2();
        }
    }
#endif

#ifndef DISABLE_ARDUINO_HWSERIAL
    // Current workaround until integrate with EventResponder.
    if (check_flags & YIELD_CHECK_HARDWARE_SERIAL) {
        HardwareSerialIMXRT::processSerialEventsList();
    }
#endif // !DISABLE_ARDUINO_HWSERIAL

    if (check_flags & YIELD_CHECK_EVENT_RESPONDER) {
        EventResponder::runFromYield();
    }

    running.store(false, std::memory_order_relaxed);
}

FLASHMEM void yield() { freertos::default_yield(); }
} // namespace freertos


/* 
 *  Trigger task to run yield() immediately. Will only
 *  work if there's no higher priority task hogging the CPU
 */
FLASHMEM void yield() {
    if (::xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED && freertos::g_yield_task) {
        if (::xPortIsInsideInterrupt() == pdTRUE) {
            BaseType_t higher_woken { pdFALSE };
            ::xTaskNotifyFromISR(freertos::g_yield_task, 0, eNoAction, &higher_woken);
            portYIELD_FROM_ISR(higher_woken);
            portDATA_SYNC_BARRIER(); // mitigate arm errata #838869
        } else {
            ::xTaskNotify(freertos::g_yield_task, 0, eNoAction);
            ::vTaskDelay(1); // as previous: could be regarded as "wrong"!
        }
    } else {
        freertos::yield();
    }
}


/*
 * Create a task to run the yield function periodically,
 * IF there are no explicit calls AND it is not blocked
 * by a CPU-hogging task of higher priority.
 */
extern void taskStart(uint32_t, const char* s="");
FLASHMEM TaskHandle_t initCustomYield(void)
{
    ::xTaskCreate(
        [](void*) {
            taskStart(YIELD_TASK_PERIOD_MS);
            while (true) {
                ::xTaskNotifyWait(0, 0, nullptr, pdMS_TO_TICKS(YIELD_TASK_PERIOD_MS));
                freertos::yield();
                yield_task_count++;
            }
        },
        PSTR("yield"), YIELD_TASK_STACK_SIZE, nullptr, YIELD_TASK_PRIORITY, &freertos::g_yield_task);

    return freertos::g_yield_task;        
}

/*
 * Execute yield function directly, bypassing any task
 * that may have been set up to execute it periodically.
 */
void yield_direct(void) { freertos::yield(); }

#else

/*
 * Functions used if we haven't set configUSE_CUSTOM_YIELD_HANDLER
 * in FreeRTOSConfig.h; just use the library defaults.
 */
FLASHMEM TaskHandle_t initCustomYield(void) { return NULL; }
void yield_direct(void) { yield(); }

#endif // defined(configUSE_CUSTOM_YIELD_HANDLER) && configUSE_CUSTOM_YIELD_HANDLER != 0
