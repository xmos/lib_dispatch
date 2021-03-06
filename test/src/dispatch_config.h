// Copyright 2021 XMOS LIMITED. This Software is subject to the terms of the 
// XMOS Public License: Version 1
#ifndef DISPATCH_CONFIG_H_
#define DISPATCH_CONFIG_H_

#if BARE_METAL

#include <stdio.h>
#include <stdlib.h>
#include <xcore/assert.h>
#include <xcore/channel.h>
#include <xcore/lock.h>

#include "debug_print.h"
#include "spinlock.h"

typedef lock_t dispatch_mutex_t;
typedef spinlock_t* dispatch_spinlock_t;

// Uncomment the following line to enable use_callers_thread mode.
//  When this is defined, the bare-metal version of the dispatch queue
//  will pitch in and execute a task in the caller's thread if all
//  workers are busy. As a result, the dispatch queue size will never
//  exceed the numebr of workers. So, be sure to set the length and
//  number of threads to be the same when creating the dispatch queue.
//  We do not recommend this mode.  Instead, we recommend that you
//  utilize the caller's thread to do other processing if you need
//  additional CPU resources.
//#define use_callers_thread

#define dispatch_assert(A) xassert(A)

#define dispatch_malloc(A) malloc(A)
#define dispatch_free(A) free(A)

#define dispatch_printf(...) debug_printf(__VA_ARGS__)

// Mutex functions
static inline dispatch_mutex_t dispatch_mutex_create() { return lock_alloc(); }
static inline void dispatch_mutex_get(dispatch_mutex_t mutex) {
  lock_acquire(mutex);
}
static inline void dispatch_mutex_put(dispatch_mutex_t mutex) {
  lock_release(mutex);
}
static inline void dispatch_mutex_delete(dispatch_mutex_t mutex) {
  lock_free(mutex);
}

// Spinlock functions
static inline dispatch_spinlock_t dispatch_spinlock_create() {
  return spinlock_create();
}
static inline void dispatch_spinlock_get(dispatch_spinlock_t lock) {
  spinlock_acquire(lock);
}
static inline void dispatch_spinlock_put(dispatch_spinlock_t lock) {
  spinlock_release(lock);
}
static inline void dispatch_spinlock_delete(dispatch_spinlock_t lock) {
  spinlock_delete(lock);
}

#elif FREERTOS

#include <xcore/assert.h>

#include "FreeRTOS.h"
#include "rtos_printf.h"
#include "semphr.h"

typedef SemaphoreHandle_t dispatch_mutex_t;

#define dispatch_assert(A) xassert(A)

#define dispatch_malloc(A) pvPortMalloc(A)
#define dispatch_free(A) vPortFree(A)

#define dispatch_printf(...) debug_printf(__VA_ARGS__)

static inline dispatch_mutex_t dispatch_mutex_create() {
  return xSemaphoreCreateMutex();
}
static inline void dispatch_mutex_get(dispatch_mutex_t mutex) {
  xSemaphoreTake(mutex, portMAX_DELAY);
}
static inline void dispatch_mutex_put(dispatch_mutex_t mutex) {
  xSemaphoreGive(mutex);
}
static inline void dispatch_mutex_delete(dispatch_mutex_t mutex) {
  vSemaphoreDelete(mutex);
}

#elif HOST

#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

typedef pthread_mutex_t *dispatch_mutex_t;

#define dispatch_assert(A) assert(A)

#define dispatch_malloc(A) malloc(A)
#define dispatch_free(A) free(A)

#define dispatch_printf(...) printf(__VA_ARGS__)

static inline dispatch_mutex_t dispatch_mutex_create() {
  pthread_mutex_t *mutex =
      (pthread_mutex_t *)dispatch_malloc(sizeof(pthread_mutex_t));
  pthread_mutex_init(mutex, NULL);
  return mutex;
}
static inline void dispatch_mutex_get(dispatch_mutex_t lock) {
  pthread_mutex_lock(lock);
}
static inline void dispatch_mutex_put(dispatch_mutex_t lock) {
  pthread_mutex_unlock(lock);
}
static inline void dispatch_mutex_delete(dispatch_mutex_t lock) {
  pthread_mutex_destroy(lock);
  dispatch_free(lock);
}

#endif

#endif  // DISPATCH_CONFIG_H_