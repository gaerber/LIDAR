/**
 * \file		hooks.c
 * \brief		FreeRTOS Hook-Functions.
 */

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"


/**
 * \fn		void vApplicationIdleHook(void)
 * \brief	An idle task hook is a function that is called during each cycle of
 * 			the idle task. If you want application functionality to run at the
 * 			idle priority then there are two options:
 *
 * 			1.) Implement the functionality in an idle task hook. There must always
 * 			be at least one task that is ready to run. It is therefore imperative
 * 			that the hook function does not call any API functions that might cause
 * 			the task to block vTaskDelay() for example.
 *
 * 			2.) Create an idle priority task to implement the functionality.
 * \return	None.
 */
//#if (configUSE_IDLE_HOOK == 1)
void vApplicationIdleHook(void)   {

}
//#endif


/**
 * \fn		void vApplicationTickHook(void)
 * \brief	The tick interrupt can optionally call an application defined hook
 * 			(or callback) function - the tick hook. The tick hook provides a
 * 			convenient place to implement timer functionality. vApplicationTickHook()
 * 			executes from within an ISR so must be very short, not use much stack,
 * 			and not call any API functions that don't end in "FromISR" or "FROM_ISR".
 * \return	None.
 */
//#if (configUSE_TICK_HOOK == 1)
void vApplicationTickHook(void)   {

}
//#endif


/**
 * \fn		void vApplicationStackOverflowHook(TaskHandle_t *pxTask, signed portCHAR *pcTaskName)
 * \brief	This Hook will be called if a stack overflow is detected. FreeRTOS
 * 			therefore provides two optional mechanisms that can be used to assist
 * 			in the detection and correction of just such an occurrence
 * 			(see www.freertos.org). Stack overflow checking introduces a context
 * 			switch overhead so its use is only recommended during the development
 * 			or testing phases.
 * \param	pxTask Handle to the task which has produced a stack overflow.
 * \param	pcTaskName Name of the task which has produced a stack overflow.
 * \return	None.
 */
//#if ((configUSE_MALLOC_FAILED_HOOK == 1) || (configCHECK_FOR_STACK_OVERFLOW == 2))
void vApplicationStackOverflowHook(TaskHandle_t *pxTask, signed portCHAR *pcTaskName) {
	for (;;) {

	}
}
//#endif


/**
 * \fn		void vApplicationMallocFailedHook(void)
 * \brief	FreeRTOS malloc fail hook. This function is alled when a malloc
 * 			fails to allocate data.
 * \return	None.
 */
//#if (configUSE_MALLOC_FAILED_HOOK == 1)
void vApplicationMallocFailedHook(void) {
	for (;;) {

	}
}
//#endif
