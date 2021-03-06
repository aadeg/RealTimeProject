/*
 * ptask.h
 * 
 * ptask containts some useful functions to handle tasks and 
 * to manipulate time
 */

#ifndef _PTASK_H_
#define _PTASK_H_

#include <time.h>
#include <pthread.h>

// ==================================================================
//                         TASK FUNCTIONS
// ==================================================================
typedef struct {
	pthread_t thread_id;
	int task_num;						// task id number
	void* arg;							// task argument
	long wcet_ms;						// worst-case execution time (ms)
	int period_ms;						// period (ms)
	int deadline_ms;					// relative deadline (ms)
	int priority;						// in [0, 99]
	int deadline_miss;					// numb. of deadline misses
	struct timespec next_activation;	// next activation time
	struct timespec abs_deadline;		// absolute deadline
} task_info_t;

int task_info_init(
		task_info_t* task, int task_num,
		int period_ms, int deadline_ms,
		int priority);
int task_deadline_missed(task_info_t* task);
int task_set_activation(task_info_t* task);
int task_wait_for_activation(task_info_t* task);

int task_create(task_info_t* task, void* (*func)(void*));
int task_join(task_info_t* task, void** return_value);


// ==================================================================
//                    		MUTEX FUNCTIONS
// ==================================================================
void ptask_mutex_init(pthread_mutex_t* mutex);


// ==================================================================
//                    TIME MANAGEMENT FUNCTIONS
// ==================================================================
void time_copy(struct timespec* des, const struct timespec* src);
void time_add_ms(struct timespec* time, int msec);
int time_cmp(const struct timespec* t1, const struct timespec* t2);

#endif