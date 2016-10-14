#ifndef __KEY_TASK_H__
#define __KEY_TASK_H__

#include <queue.h>
#include <stdbool.h>

typedef enum {
    KEY_0 = 0,
    KEY_1,
    KEY_2,
} key_enum_t;

typedef struct {
    key_enum_t key;
    bool on;
} key_event_t;

extern xQueueHandle key_queue;

void key_task_init();

#endif /* end of include guard: __KEY_TASK_H__ */
