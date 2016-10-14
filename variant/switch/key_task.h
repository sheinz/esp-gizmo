#ifndef __KEY_TASK_H__
#define __KEY_TASK_H__

#include <queue.h>
#include <stdbool.h>

typedef struct {
    uint8_t key_index;
    bool on;
} key_event_t;

extern xQueueHandle key_queue;

void key_task_init();

#endif /* end of include guard: __KEY_TASK_H__ */
