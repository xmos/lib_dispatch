// Copyright (c) 2020, XMOS Ltd, All rights reserved
#include "lib_dispatch/api/dispatch_queue_host.h"

#include <cassert>
#include <cstdlib>
#include <cstring>

#include "lib_dispatch/api/dispatch_group.h"
#include "lib_dispatch/api/dispatch_queue.h"
#include "lib_dispatch/api/dispatch_task.h"

#define DISPATCH_TASK_NONE (0)

void dispatch_thread_handler(dispatch_host_queue_t *queue, int *task_id) {
  std::unique_lock<std::mutex> lock(queue->lock);

  do {
    // Wait until we have data or a quit signal
    queue->cv.wait(lock,
                   [queue] { return (queue->deque.size() || queue->quit); });

    // after wait, we own the lock
    if (!queue->quit && queue->deque.size()) {
      dispatch_task_t &task = queue->deque.front();

      // set the current task
      *task_id = task.id;

      queue->deque.pop_front();

      // unlock now that we're done messing with the queue
      lock.unlock();

      dispatch_task_perform(&task);

      // reset current task
      *task_id = DISPATCH_TASK_NONE;

      lock.lock();
    }
  } while (!queue->quit);
}

dispatch_queue_t *dispatch_queue_create(size_t length, size_t thread_count,
                                        size_t stack_size, const char *name) {
  dispatch_host_queue_t *queue;

  std::printf("dispatch_queue_create: length=%d, thread_count=%d\n", length,
              thread_count);

  queue = new dispatch_host_queue_t;
  queue->threads.resize(thread_count);
  queue->thread_task_ids.resize(thread_count);

  if (name)
    queue->name.assign(name);
  else
    queue->name.assign("null");

  // initialize the queue
  dispatch_queue_init(queue);

  std::printf("dispatch_queue_create: name=%s\n", queue->name.c_str());

  return queue;
}

void dispatch_queue_init(dispatch_queue_t *ctx) {
  assert(ctx);
  dispatch_host_queue_t *queue = static_cast<dispatch_host_queue_t *>(ctx);

  std::printf("dispatch_queue_init: name=%s\n", queue->name.c_str());

  queue->quit = false;
  for (size_t i = 0; i < queue->threads.size(); i++) {
    queue->thread_task_ids[i] = DISPATCH_TASK_NONE;
    queue->threads[i] = std::thread(&dispatch_thread_handler, queue,
                                    &queue->thread_task_ids[i]);
  }
}

void dispatch_queue_async_task(dispatch_queue_t *ctx, dispatch_task_t *task) {
  assert(ctx);
  assert(task);
  dispatch_host_queue_t *queue = static_cast<dispatch_host_queue_t *>(ctx);

  std::printf("dispatch_queue_async_task: name=%s\n", queue->name.c_str());

  // assign to this queue
  task->queue = static_cast<dispatch_queue_struct *>(ctx);

  std::unique_lock<std::mutex> lock(queue->lock);
  queue->deque.push_back(*task);
  // manual unlocking is done before notifying, to avoid waking up
  // the waiting thread only to block again (see notify_one for details)
  lock.unlock();

  queue->cv.notify_one();
}

void dispatch_queue_wait(dispatch_queue_t *ctx) {
  assert(ctx);
  dispatch_host_queue_t *queue = static_cast<dispatch_host_queue_t *>(ctx);

  std::printf("dispatch_queue_wait: name=%s\n", queue->name.c_str());

  int busy_count = 0;

  for (;;) {
    std::unique_lock<std::mutex> lock(queue->lock);
    busy_count = queue->deque.size();  // tasks on the queue are considered busy
    lock.unlock();
    for (int i = 0; i < queue->threads.size(); i++) {
      if (queue->thread_task_ids[i] != DISPATCH_TASK_NONE) busy_count++;
    }
    if (busy_count == 0) return;
  }
}

void dispatch_queue_task_wait(dispatch_queue_t *ctx, int task_id) {
  assert(ctx);
  assert(task_id > DISPATCH_TASK_NONE);

  dispatch_host_queue_t *queue = (dispatch_host_queue_t *)ctx;

  bool done_waiting = true;

  for (;;) {
    done_waiting = true;
    std::unique_lock<std::mutex> lock(queue->lock);
    for (int i = 0; i < queue->deque.size(); i++) {
      dispatch_task_t &queued_task = queue->deque.at(i);

      if (queued_task.id == task_id) {
        done_waiting = false;
        break;
      }
    }
    lock.unlock();
    for (int i = 0; i < queue->threads.size(); i++) {
      if (queue->thread_task_ids[i] == task_id) {
        done_waiting = false;
        break;
      }
    }
    if (done_waiting) break;
  }
}

void dispatch_queue_destroy(dispatch_queue_t *ctx) {
  assert(ctx);
  dispatch_host_queue_t *queue = static_cast<dispatch_host_queue_t *>(ctx);

  std::printf("dispatch_queue_destroy: name=%s\n", queue->name.c_str());

  // signal to all thread workers that it is time to quit
  std::unique_lock<std::mutex> lock(queue->lock);
  queue->quit = true;
  lock.unlock();
  queue->cv.notify_all();

  // Wait for threads to finish before we exit
  for (size_t i = 0; i < queue->threads.size(); i++) {
    if (queue->threads[i].joinable()) {
      // printf("Destructor: Joining thread %zu until completion\n", i);
      queue->threads[i].join();
    }
  }

  // free memory
  delete queue;
}