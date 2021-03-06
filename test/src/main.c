// Copyright 2021 XMOS LIMITED. This Software is subject to the terms of the 
// XMOS Public License: Version 1
#include <stdlib.h>

#if FREERTOS
#include "FreeRTOS.h"
#include "task.h"
void vApplicationMallocFailedHook(void) {
  debug_printf("Malloc failed!\n");
  exit(1);
}
#endif

#include "unity.h"
#include "unity_fixture.h"

#if FREERTOS
static void RunTests(void* unused) {
  RUN_TEST_GROUP(dispatch_task);
  RUN_TEST_GROUP(dispatch_group);
  RUN_TEST_GROUP(dispatch_queue);
  RUN_TEST_GROUP(dispatch_queue_rtos);
  UnityEnd();
  exit(Unity.TestFailures);
}
#endif

#if defined(BARE_METAL)
static void RunTests(void* unused) {
  RUN_TEST_GROUP(dispatch_task);
  RUN_TEST_GROUP(dispatch_group);
  RUN_TEST_GROUP(dispatch_queue);
  RUN_TEST_GROUP(queue_metal);
  RUN_TEST_GROUP(dispatch_queue_metal);
  UnityEnd();
}
#endif

#if defined(HOST)
static void RunTests(void* unused) {
  RUN_TEST_GROUP(dispatch_task);
  RUN_TEST_GROUP(dispatch_group);
  RUN_TEST_GROUP(dispatch_queue);
  UnityEnd();
}
#endif

int main(int argc, const char* argv[]) {
  UnityGetCommandLineOptions(argc, argv);
  UnityBegin(argv[0]);

#if FREERTOS
  xTaskCreate(RunTests, "RunTests", 1024 * 16, NULL, configMAX_PRIORITIES - 1,
              NULL);
  vTaskStartScheduler();
#endif
  // for FreeRTOS build we never reach here because vTaskStartScheduler never
  // returns
  RunTests(NULL);
  return (int)Unity.TestFailures;
}