/*
 * structs.h
 * 
 * Constaints the definition of the structures used in the code
 */

#ifndef _STRUCTS_H_
#define _STRUCTS_H_

#include <stdbool.h>
#include <pthread.h>

#include "./consts.h"

// ==================================================================
//                         ENUM DEFINITION
// ==================================================================
enum airplane_status {
	INBOUND_HOLDING,
	INBOUND_LANDING,
	OUTBOUND_HOLDING,
	OUTBOUND_TAKEOFF
};

// ==================================================================
//                    STRUCTURES DEFINITION
// ==================================================================
// Point of the desired trajectory that an airplane has to follow 
typedef struct {
	float x;		// x cartesian coordinate
	float y;		// y cartesian coordinate
	float vel;		// velocity
} waypoint_t;

// Desired trajectory that an airplane has to follow
typedef struct {
	waypoint_t waypoints[MAX_WAYPOINTS]; // list of points
	bool is_cyclic;						 // true if the trajectory is cyclic
	int size;							 // number of points in the trajectory
} trajectory_t;

// Contain all the information related to an airplane
typedef struct {
	float x;						// current x cartisian coordinate
	float y;						// current x cartisian coordinate
	float angle;					// current angle in radiants
	float vel;						// current velocity
	const trajectory_t* des_traj;	// pointer to the desired trajectory
	int traj_index;		// the index of the active point on the trajectory
	bool traj_finished; // true the desired trajectory doesn't have new points
	enum airplane_status status;	// status of the airplane
	int unique_id;					// incremental index
	bool kill;						// true if the airplane must be despawned
} airplane_t;

// Put together the airplane struct with its mutex
typedef struct {
	airplane_t airplane;
	pthread_mutex_t mutex;
} shared_airplane_t;

// 2D Point with Integer coordinates
typedef struct {
	int x;
	int y;
} point2i_t;

// Cyclic buffer used to store airplanes' trails
typedef struct {
	point2i_t points[TRAIL_BUFFER_LENGTH];
	int top;	// index of the latest element inserted in the buffer
} cbuffer_t;

// Queue of airplanes
typedef struct {
	shared_airplane_t* elems[AIRPLANE_QUEUE_LENGTH];
	int top;		// Index of the first element in the queue
	int bottom;		// Index of the first free element
	pthread_mutex_t mutex;
} airplane_queue_t;

// Airplane pool used for the allocation of new airplane structures
typedef struct {
	shared_airplane_t elems[AIRPLANE_POOL_SIZE];
	// true if the corresponding element is not used
	bool is_free[AIRPLANE_POOL_SIZE];
	int n_free;				// number of free elements
	pthread_mutex_t mutex;
} airplane_pool_t;

// Contain all the information used in the section SYSTEM STATE of the sidebar
typedef struct {
	int n_airplanes;					// number of airplanes in the system
	bool is_runway_free[N_RUNWAYS];		// state of the runways
	bool random_gen_enabled;			// state of the random generation
} system_state_t;

typedef struct {
	system_state_t state;
	pthread_mutex_t mutex;
} shared_system_state_t;

// Contain all the information used in the section TASKS STATE of the sidebar
typedef struct {
	char str[TASK_NAME_LENGTH];			// display name of the task
	int deadline_miss;					// number of deadline misses
	bool is_running;					// true if the task is running
} task_state_t;


// ==================================================================
//                    FUNCTION DEFINITION
// ==================================================================
// Airplane pool
void airplane_pool_init(airplane_pool_t* pool);
shared_airplane_t* airplane_pool_get_new(airplane_pool_t* pool);
void airplane_pool_free(airplane_pool_t* pool, shared_airplane_t* elem);

// Airplanes queue
void airplane_queue_init(airplane_queue_t* queue);
int airplane_queue_push(airplane_queue_t* queue, shared_airplane_t* new_value);
shared_airplane_t* airplane_queue_pop(airplane_queue_t* queue);
bool airplane_queue_is_empty(airplane_queue_t* queue);
bool airplane_queue_is_full(airplane_queue_t* queue);

// Cyclic buffer
void cbuffer_init(cbuffer_t* buffer);
int cbuffer_next_index(cbuffer_t* buffer);

// Trajectory
const waypoint_t* trajectory_get_point(const trajectory_t* trajectory, int index);

#endif
