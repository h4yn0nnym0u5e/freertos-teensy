# customYield example

Some notes on how to use the `customYield` sketch to investigate the effects of various built-in and customised `yield()` behaviours.

## Preamble
### Teensyduino
The `yield()` function is called on every iteration of `loop()`, every millisecond during `delay()`, or if explicitly called. 

It is _also_ called during some time-consuming library functions, which is poorly documented and may be undesirable, but we're stuck with it.

### freertos-teensy
The `yield()` function is called inside the idle task, unless a custom handler has been configured. The idle task runs at priority 0, i.e. only if no other tasks of higher priority are running. It can also be explicitly called from any task, of course.

As an RTOS, `loop()` is no longer relevant, and the use of `delay` should be replaced by an equivalent call to `vTaskDelay()`.

## Example sketch
Note that this sketch is _not_ intended to be an example of good practice! It runs three or four tasks, in addition to the `idle` task:
| name | priority | function |
| --- | --- | --- |
| `blink` | 2 | blinks built-in LED |
| `ticky` | 1 | prints TICK TOCK repeatedly to `Serial`; may hog CPU |
| `newln` | 2 | prints info to `Serial` every 5 seconds |
| `yield`* | 0 | calls `yield()` at configured period, or if explicitly triggered |

\* only if enabled by use of custom yield handler

### Customisation options
Various customisation options are available to allow quick changes to the example's performance so you can see the result.
| option | where | default | alternative | meaning |
| ---- | ---- | --- | --- | --- |
| configUSE_CUSTOM_YIELD_HANDLER | FreeRTOSConfig.h | 0 | 1 | enable handler provided in yieldStuff.cpp |
| CPU hogging | main.cpp | noCPU_HOG | CPU_HOG | make `ticky` task hog CPU rather than suspending between actions |
| YIELD_TASK_PRIORITY | yieldStuff.h | 0 | values up to configMAX_PRIORITIES - 1 | set `yield` task's priority |
| YIELD_TASK_PERIOD_MS | yieldStuff.h | 10 | to suit you | set `yield()` call minimum period |


| custom <br> yield|hog <br> CPU|yield <br> task <br> priority|period <br> ms| result |
| :---: | :---: |:---: | :---: | --- | 
| 0 | no | n/a | n/a | idle task runs OK; `yield` occurs often |
| 0 | yes | n/a | n/a | idle task never runs; `yield` occurs only if explicitly called |
| 1 | no | 0 | 10 | idle task runs OK; `yield` occurs at `YIELD_TASK_PERIOD_MS` |
| 1 | yes | 0 | 10 | idle task never runs; `yield` occurs only if explicitly called |
| 1 | yes | 1 | 10 | idle task never runs; `yield` occurs at `YIELD_TASK_PERIOD_MS` |

### yieldStuff.cpp
The global scope `::yield()` function is copied from the previous generation of `freertos-teensy`, and inserts a `vTaskDelay(1)` call whenever it is executed (not from an ISR, and with the scheduler running). This was intended to allow the yield task to run, but had the side effect of inserting a 1ms delay into some library calls, which is not always desirable.
