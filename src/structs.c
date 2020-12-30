#include <assert.h>

#include "structs.h"

// ==================================================================
//                         AIRPLANE POOL
// ==================================================================
// Initialize the airplane pool
void airplane_pool_init(airplane_pool_t* pool) {
	int i = 0;

	for (i = 0; i < AIRPLANE_POOL_SIZE; ++i) {
		pool->elems[i].airplane.unique_id = i;
		pool->is_free[i] = true;
	}

	pthread_mutex_init(&pool->mutex, NULL);
	pool->n_free = AIRPLANE_POOL_SIZE;
}

// Retrieve a free airplane from the pool. If the pool doesn't have
// a free airplane, NULL is returned
shared_airplane_t* airplane_pool_get_new(airplane_pool_t* pool) {
	shared_airplane_t* elem = NULL;
	int i = 0;

	pthread_mutex_lock(&pool->mutex);
	// Searching for the first free element
	while (i < AIRPLANE_POOL_SIZE && pool->is_free[i] == false) {
		++i;
	}

	if (i < AIRPLANE_POOL_SIZE) {		// Free element found
		// Updating the pool
		elem = &pool->elems[i];
		pool->is_free[i] = false;
		--pool->n_free;
	}
	pthread_mutex_unlock(&pool->mutex);
	return elem;
}

// Set an airplane as free and ready to be recycled
void airplane_pool_free(airplane_pool_t* pool, shared_airplane_t* elem) {
	// Checking if "elem" points to a valid location of the pool
	assert(elem >= &(pool->elems[0]) && elem < &(pool->elems[AIRPLANE_POOL_SIZE]));
	int i = elem - &pool->elems[0];		// Index of the element

	pthread_mutex_lock(&pool->mutex);
	// Updating the pool
	pool->is_free[i] = true;
	++pool->n_free;
	pthread_mutex_unlock(&pool->mutex);
}

// ==================================================================
//                         AIRPLANE QUEUE
// ==================================================================
// Initialize an airplane queue
void airplane_queue_init(airplane_queue_t* queue) {
	queue->top = 0;
	queue->bottom = 0;
	pthread_mutex_init(&queue->mutex, NULL);
}

// Check if the queue is empty. Thread UNSAFE function. Internal use only
bool _airplane_queue_is_empty_unsafe(const airplane_queue_t* queue) {
	return queue->top == queue->bottom;
}

// Check if the queue is full. Thread UNSAFE function. Internal use only
bool _airplane_queue_is_full_unsafe(const airplane_queue_t* queue) {
	return ((queue->bottom + 1) % AIRPLANE_QUEUE_LENGTH) == queue->top;
}

// Push a new element in the queue. ERROR_GENERIC is returned if the queue is full
int airplane_queue_push(airplane_queue_t* queue, shared_airplane_t* new_value){
	int rv = SUCCESS;		// Return value

	pthread_mutex_lock(&queue->mutex);
	if (!_airplane_queue_is_full_unsafe(queue)) {
		queue->elems[queue->bottom] = new_value;
		queue->bottom = (queue->bottom + 1) % AIRPLANE_QUEUE_LENGTH;
		rv = SUCCESS;
	} else {
		rv = ERROR_GENERIC;
	}
	pthread_mutex_unlock(&queue->mutex);
	return rv;
}

// Pop the first element from the queue. NULL is returned if the queue is empty
shared_airplane_t* airplane_queue_pop(airplane_queue_t* queue){
	shared_airplane_t* elem = NULL;

	pthread_mutex_lock(&queue->mutex);
	if (!_airplane_queue_is_empty_unsafe(queue)) {
		elem = queue->elems[queue->top];
		queue->elems[queue->top] = NULL;
		queue->top = (queue->top + 1) % AIRPLANE_QUEUE_LENGTH;
	}
	pthread_mutex_unlock(&queue->mutex);
	return elem;
}

// Check if the queue is empty. Thead safe function
bool airplane_queue_is_empty(airplane_queue_t* queue) {
	bool is_empty = false;
	pthread_mutex_lock(&queue->mutex);
	is_empty = _airplane_queue_is_empty_unsafe(queue);
	pthread_mutex_unlock(&queue->mutex);
	return is_empty;
}

// Check if the queue is full. Thead safe function
bool airplane_queue_is_full(airplane_queue_t* queue) {
	bool is_full = false;
	pthread_mutex_lock(&queue->mutex);
	is_full = _airplane_queue_is_full_unsafe(queue);
	pthread_mutex_unlock(&queue->mutex);
	return is_full;
}

// ==================================================================
//                         CYCLIC BUFFER
// ==================================================================
// Initialize the cbuffer
void cbuffer_init(cbuffer_t* buffer) {
	int i = 0;
	buffer->top = 0;

	// Initializing all the points
	for (i = 0; i < TRAIL_BUFFER_LENGTH; ++i) {
		buffer->points[i].x = 0;
		buffer->points[i].y = 0;
	}
}

// Return the index of the next location and increment the top index
int cbuffer_next_index(cbuffer_t* buffer) {
	int index = (buffer->top + 1) % TRAIL_BUFFER_LENGTH;
	++buffer->top;
	return index;
}

// ==================================================================
//                           TRAJECTORY
// ==================================================================
// Return the waypoint at position "index". If the trajectory is cyclic, the index
// is bounded to the trajectory size
const waypoint_t* trajectory_get_point(const trajectory_t* trajectory, int index) {
	if (trajectory == NULL) return NULL;
	if (index >= 0 && index < trajectory->size) {
		return &trajectory->waypoints[index];
	}
	if (trajectory->is_cyclic) {
		index = index % trajectory->size;
		return &trajectory->waypoints[index];
	}

	return NULL;
}