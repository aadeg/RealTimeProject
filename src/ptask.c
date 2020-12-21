#include <time.h>
#include <stdbool.h>

#include "ptask.h"
#include "consts.h"

#define MS_IN_SEC 1000
#define NSEC_IN_MS 1000000
#define NSEC_IN_SEC (MS_IN_SEC * NSEC_IN_MS)

#define MIN_PRIORITY 0
#define MAX_PRIORITY 99

/*
 =================================================================
												TASK FUNCTIONS
 =================================================================
*/

// Initializes the task_info struct
//
// task_num: 		task id number choose by the user of the function
// period_ms: 	task period (ms)
// deadline_ms:	relative deadline (ms)
// priority:		in [0, 99]
//
// return SUCCESS or ERROR_GENERIC
int task_info_init(
		struct task_info* task,
		int task_num,
		int period_ms, int deadline_ms,	int priority) {
	// Checking the arguments
	if (priority < MIN_PRIORITY || priority > MAX_PRIORITY) return ERROR_GENERIC;
	if (period_ms <= 0 || deadline_ms <= 0) return ERROR_GENERIC;
	
	task->arg = NULL;
	task->task_num = task_num;
	task->wcet_ms = 0;
	task->period_ms = period_ms;
	task->deadline_ms = deadline_ms;
	task->priority = priority;
	task->deadline_miss = 0;
	return SUCCESS;
}

// Checks whether or not a deadline miss has occurred
// return true if a deadline miss has occurred
int task_deadline_missed(struct task_info* task) {
	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);
	
	if (time_cmp(&now, &task->abs_deadline) > 0) {
		++(task->deadline_miss);
		return true;
	}
	return false;
}

// Set the next activation time and the absolute deadline
// return SUCCESS or ERROR_GENERIC
int task_set_activation(struct task_info* task) {
	int err;
	struct timespec now;

	err = clock_gettime(CLOCK_MONOTONIC, &now);
	if (err) return ERROR_GENERIC;

	time_copy(&task->next_activation, &now);
	time_add_ms(&task->next_activation, task->period_ms);

	time_copy(&task->abs_deadline, &now);
	time_add_ms(&task->abs_deadline, task->deadline_ms);
	return SUCCESS;
}

// Suspend the task until the next activation
// return SUCCESS or ERROR_GENERIC
int task_wait_for_activation(struct task_info* task) {
	int err;

	err = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME,
			&task->next_activation, NULL);
	if (err) return ERROR_GENERIC;

	time_add_ms(&task->next_activation, task->period_ms);
	time_add_ms(&task->abs_deadline, task->period_ms);
	return SUCCESS;
}

// Creates a new task using with SCHED_FIFO as scheduler
// return SUCCESS or ERROR_GENERIC
int task_create(struct task_info* task_info, void* (*func)(void*)) {
	int err;
	pthread_attr_t attr;
	struct sched_param s_param;

	// Setting pthread attributes
	s_param.sched_priority = task_info->priority;
	err = pthread_attr_init(&attr);
	if (err) return ERROR_GENERIC;
	
	err = pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
	if (!err) err |= pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
	if (!err) err |= pthread_attr_setschedparam(&attr, &s_param);

	// Creating the thread
	if (!err) err |= pthread_create(&task_info->thread_id, &attr, func, task_info);

	// Cleanup
	err |= pthread_attr_destroy(&attr);
	return (err) ? ERROR_GENERIC : SUCCESS;
}

// Join a task as using pthread_joint
// return the value returned by pthread_join
int task_join(struct task_info* task_info, void** return_value) {
	return pthread_join(task_info->thread_id, return_value);
}

/*
 =================================================================
										TIME MANAGEMENT FUNCTIONS
 =================================================================
*/
// Copies a struct timespec from des to src
void time_copy(struct timespec* des, const struct timespec* src) {
	des->tv_sec = src->tv_sec;
	des->tv_nsec = src->tv_nsec;
}

// Adds msec to time
void time_add_ms(struct timespec* time, int msec) {
	time->tv_sec += msec / MS_IN_SEC;
	time->tv_nsec += (msec % MS_IN_SEC) * NSEC_IN_MS;

	if (time->tv_nsec > NSEC_IN_SEC) {
		time->tv_nsec -= NSEC_IN_SEC;
		time->tv_sec += 1;
	}
}

// return:
//   0 if t1 == t2
//   1 if t1 > t2
//   1 if t2 < t1
int time_cmp(const struct timespec* t1, const struct timespec* t2) {
	if (t1->tv_sec > t2->tv_sec) return 1;
	if (t1->tv_sec < t2->tv_sec) return -1;
	if (t1->tv_nsec > t2->tv_nsec) return 1;
	if (t1->tv_nsec < t2->tv_nsec) return -1;
	return 0;
}