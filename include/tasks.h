#ifndef INCLUDE_TASKS_H
#define INCLUDE_TASKS_H
#include "cpu_state.h"
#include "stddef.h"
#include "stdint.h"

#define MAX_CONCURRENT_TASKS 64

/*
READY = default, ready to run.
WAITING = in sleep().
RUNNING = being executed by cpu.
TERMINATED = function exited.
*/
enum TASK_STATE { TASK_READY = 0, TASK_WAITING, TASK_RUNNING, TASK_TERMINATED };

/*
JOINABLE = function has returned, we may terminate task.
DETACHED = parent task no longer waits for this task.
*/
enum JOIN_STATE { JOIN_JOINABLE, JOIN_DETACHED };

struct task_struct {
    struct cpu_state cpu_state;      // 32 bytes
    struct stack_state stack_state;  // 16 bytes
    uint32_t pid;                    // This got changed
    enum TASK_STATE state;
    int32_t static_prio;
    int32_t dynamic_prio;   // How long current task has been running. Add to priority for real priority.
    void* stack_bottom;     // Keep addr to free on task termination, and stack overflow detection.
    size_t stack_size;
    int32_t interruptible;  // If zero then current task on local CPU will not be interrupted by timer ISR.
    enum JOIN_STATE join_state;
    int32_t sleep_avg;      // Average time this task spends sleeping, used to determine whether a task is I/O-bound or CPU-bound. Boost prio of interactive tasks.
};

// Create a new task.
struct task_struct* task_new(void (*fp)(void*), void* arg, size_t stack_size, int32_t priority);

// Join a running task, release its resources.
void task_join(struct task_struct*);

// Detach a running task.
void task_detach(struct task_struct*);

// Give up control to another process.
void task_yield();

void test_caller();

// Get number of running tasks.
int32_t task_get_nr();

// Called by PIT INT_SYSTIMER.
void task_isr_tick();

// Get current running task.
struct task_struct* task_get_current();

void task_set_current(struct task_struct* t);

// Get current running task's pid.
uint32_t task_getpid();

void tasks_init();

#endif
