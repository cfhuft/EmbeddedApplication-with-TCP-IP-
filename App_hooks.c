/*
 * App_hooks.c
 *
 *  Created on: Jul 30, 2021
 *      Author: Samo Novak
 */

#include <App_hooks.h>


void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin){

	if (GPIO_Pin==GPIO_PIN_1){
			BaseType_t xHigherPriorityTaskWoken = pdFALSE;
			ISR1_count++;
			static Event const ETHERNET_Event = {ETHERNET_READ_SIG};
			Active_postFromISR(ETHERNET_ptr, &ETHERNET_Event, &xHigherPriorityTaskWoken);
		}
}


int32_t constrain(int32_t x, int32_t high, int32_t low){
	if (x>high){return high;}
	else if (x<low){return low;}
	else return x;
}

// Only the "FromISR" API variants are allowed in vApplicationTickHook
//vApplicationTickHook is called with the tick interrupt (set to fire every 1ms in RTOS config)
void vApplicationTickHook(void) {

    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    TimeEvent_tickFromISR(&xHigherPriorityTaskWoken);
    portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
}

//The idle task can optionally call an application defined hook (or callback) function - the idle hook. The idle task runs at the very lowest priority,
//so such an idle hook function will only get executed when there are no tasks of higher priority that are able to run.
void vApplicationIdleHook() {
#ifdef NDEBUG
    __WFI(); 	// Wait-For-Interrupt
#endif
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
    (void)xTask;
    (void)pcTaskName;
    /* ERROR */
}

//configSUPPORT_STATIC_ALLOCATION is set to 1, so the application must
//provide an implementation of vApplicationGetIdleTaskMemory() to provide
//the memory that is used by the Idle task.
void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer,
                                   StackType_t **ppxIdleTaskStackBuffer,
                                   uint32_t *pulIdleTaskStackSize )
{

    static StaticTask_t xIdleTaskTCB;
    static StackType_t uxIdleTaskStack[configMINIMAL_STACK_SIZE];

    //Pass out a pointer to the StaticTask_t structure in which the
    //	Idle task's state will be stored.
    *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;

    // Pass out the array that will be used as the Idle task's stack
    *ppxIdleTaskStackBuffer = uxIdleTaskStack;

    // Pass out the size of the array pointed to by *ppxIdleTaskStackBuffer.
    *pulIdleTaskStackSize = sizeof(uxIdleTaskStack)/sizeof(uxIdleTaskStack[0]);
}

void Q_onAssert(char const *module, int loc) {
    /*application-specific error handling */
    (void)module;
    (void)loc;
#ifndef NDEBUG /* debug build? */

    /* tie the CPU in this endless loop and wait for the debugger... */
    while (1) {
    }
#else /* production build */
    /* TODO: do whatever is necessary to put the system in a fail-safe state */
    NVIC_SystemReset(); /* reset the CPU */
#endif
}
