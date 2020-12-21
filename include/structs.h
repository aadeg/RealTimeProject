/*
 * structs.h
 * 
 * Constaints the definition of the structures used in the code
 */

#ifndef _STRUCTS_H_
#define _STRUCTS_H_

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
	float x;		// (m)
	float y;		// (m)
	float angle;	// (rad)
	float vel;		// (m/s)
} waypoint_t;

// Desired trajectory that an airplane has to follow
typedef struct {
	waypoint_t waypoints[MAX_WAYPOINTS]; // list of points
	bool is_cyclic;						 // true if the trajectory is cyclic
	int size;							 // number of points in the trajectory
} trajectory_t;

// Contains the state of the airplane and the desired trajectory to be followed
typedef struct {
	float x;			// (m)
	float y;			// (m)
	float angle;		// (rad)
	float vel;			// (m/s)
	const trajectory_t* des_traj;
	int traj_index;		// the index of the active point of the trajectory
	bool traj_finished; // true the desired trajectory doesn't have new points
	enum airplane_status status;
} airplane_t;

// Puts together the airplane struct with its mutex
typedef struct {
	airplane_t airplane;
	pthread_mutex_t mutex;
} shared_airplane_t;

// 2D Point with Interger coordinates
typedef struct {
	int x;
	int y;
} point2i_t;

// Cyclic buffer used to store airplanes' trails
typedef struct {
	point2i_t points[TRAIL_BUFFER_LENGTH];
	int top;	// index of the latest element of the buffer
} cbuffer_t;

typedef struct {
	int indexes[AIRPLANE_QUEUE_LENGTH];
	int top;
	int bottom;
} airplane_queue_t;

#endif
