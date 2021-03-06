// Copyright 2021 XMOS LIMITED. This Software is subject to the terms of the 
// XMOS Public License: Version 1
#include "dispatch_group.h"

#include <stdlib.h>
#include <string.h>

#include "dispatch_config.h"
#include "dispatch_types.h"

dispatch_group_t *dispatch_group_create(size_t length, bool waitable) {
  dispatch_group_t *group;

  dispatch_printf("dispatch_group_create: length=%d\n", length);

  group = dispatch_malloc(sizeof(dispatch_group_t));

  group->length = length;
  group->tasks = dispatch_malloc(sizeof(dispatch_task_t *) * length);

  // initialize the queue
  dispatch_group_init(group, waitable);

  return group;
}

void dispatch_group_init(dispatch_group_t *group, bool waitable) {
  dispatch_assert(group);

  group->waitable = waitable;
  group->count = 0;
}

dispatch_task_t *dispatch_group_function_add(dispatch_group_t *group,
                                             dispatch_function_t function,
                                             void *argument) {
  dispatch_task_t *task;

  task = dispatch_task_create(function, argument, group->waitable);
  dispatch_group_task_add(group, task);

  return task;
}

void dispatch_group_task_add(dispatch_group_t *group, dispatch_task_t *task) {
  dispatch_assert(group);
  dispatch_assert(group->count < group->length);

  task->waitable = group->waitable;
  group->tasks[group->count] = task;
  group->count++;
}

void dispatch_group_perform(dispatch_group_t *group) {
  dispatch_assert(group);

  dispatch_printf("dispatch_group_perform: %u\n", (size_t)group);

  // call group in current thread
  for (int i = 0; i < group->count; i++) {
    dispatch_task_t *task = group->tasks[i];
    dispatch_task_perform(task);
  }
}

void dispatch_group_delete(dispatch_group_t *group) {
  dispatch_assert(group);
  dispatch_assert(group->tasks);

  dispatch_free(group->tasks);
  dispatch_free(group);
}
