#if !defined(_YIELDSTUFF_H_)
#define _YIELDSTUFF_H_

#define YIELD_TASK_PERIOD_MS   10
//*
#define YIELD_TASK_PRIORITY     1
/*/
#define YIELD_TASK_PRIORITY     (configMAX_PRIORITIES - 1)  
// */
#define YIELD_TASK_STACK_SIZE 512

extern volatile uint32_t idle_count;
extern volatile uint32_t yield_count;
extern volatile uint32_t yield_task_count;
extern void initYieldEvent(void);
extern TaskHandle_t initCustomYield(void);
extern void yield_direct(void);
extern uint32_t measureYieldDuration(void);
namespace freertos { 
  extern TaskHandle_t g_yield_task; 
}
#endif // !defined(_YIELDSTUFF_H_)
