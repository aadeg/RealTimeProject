/*
 * consts.h
 * 
 * Constaints all the constants used in the code
 */

#ifndef _CONSTS_H_
#define _CONSTS_H_

// ==================================================================
//                            GRAPHIC CONSTANTS
// ==================================================================
#define SCREEN_WIDTH			1024
#define SCREEN_HEIGHT			720
#define BG_COLOR				0     	// black
#define MAIN_COLOR				14		// yellow

// Main Box
#define MAIN_BOX_WIDTH			(SCREEN_HEIGHT - 20)
#define MAIN_BOX_HEIGHT			(SCREEN_HEIGHT - 20)
#define MAIN_BOX_X				10
#define MAIN_BOX_Y				10

// Status Box
#define STATUS_BOX_WIDTH		(SCREEN_WIDTH - SCREEN_HEIGHT - 20)
#define STATUS_BOX_HEIGHT		(SCREEN_HEIGHT - 20)
#define STATUS_BOX_X			(SCREEN_HEIGHT + 10)
#define STATUS_BOX_Y			10
#define STATUS_BOX_PADDING 		5

// Airplane
#define AIRPLANE_SIZE			10
#define AIRPLANE_COLOR			11
#define AIRPLANE_TRAIL_COLOR	10

// ==================================================================
//                     AIRPLANE TASK CONSTANTS
// ==================================================================
#define AIRPLANE_CTRL_OMEGA_GAIN	0.8
#define AIRPLANE_CTRL_VEL_GAIN		1.0
#define AIRPLANE_CTRL_MIN_DIST		20.0
#define AIRPLANE_CTRL_VEL			15.0
#define AIRPLANE_CTRL_SIM_PERIOD	0.020

// ==================================================================
//                     TRAJECTORY CONSTANTS
// ==================================================================
#define HOLDING_TRAJECTORY_SIZE 		30
#define HOLDING_TRAJECTORY_STEP 		(2.0 * M_PI / HOLDING_TRAJECTORY_SIZE)
#define HOLDING_TRAJECTORY_RADIUS 		50.0
#define HOLDING_TRAJECTORY_ARM			70.0
#define HOLDING_TRAJECTORY_X			-150.0
#define HOLDING_TRAJECTORY_Y			150.0

#define RUNWAY_0_LANDING_TRAJ_SIZE		10
#define RUNWAY_0_LANDING_TRAJ_START_X	-100.0
#define RUNWAY_0_LANDING_TRAJ_START_Y	-100.0
#define RUNWAY_0_LANDING_TRAJ_END_X		100.0
#define RUNWAY_0_LANDING_TRAJ_END_Y		-100.0
#define RUNWAY_0_LANDING_TRAJ_END_VEL	5.0

#define RUNWAY_1_LANDING_TRAJ_SIZE		10
#define RUNWAY_1_LANDING_TRAJ_START_X	-100.0
#define RUNWAY_1_LANDING_TRAJ_START_Y	-50.0
#define RUNWAY_1_LANDING_TRAJ_END_X		100.0
#define RUNWAY_1_LANDING_TRAJ_END_Y		-50.0
#define RUNWAY_1_LANDING_TRAJ_END_VEL	5.0


// ==================================================================
//               AIR TRAFFIC CONTROLLER TASK CONSTANTS
// ==================================================================
#define N_RUNWAYS		2
#define FREE_RUNWAY		-1

// ==================================================================
//                     ARRAYS MAX LENGTH
// ==================================================================
#define MAX_AIRPLANE			10
#define AIRPLANE_POOL_SIZE		MAX_AIRPLANE
#define TRAIL_BUFFER_LENGTH		100
#define MAX_WAYPOINTS 			50
#define AIRPLANE_QUEUE_LENGTH	(MAX_AIRPLANE + 1)

// ==================================================================
//                     SCHEDULING CONSTANTS
// ==================================================================
#define GRAPHIC_PERIOD_MS		30
#define INPUT_PERIOD_MS			30
#define AIRPLANE_PERIOD_MS 		20
#define TRAFFIC_CTRL_PERIOD_MS	5
#define PRIORITY      			50

// ==================================================================
//                     		UTILITIES
// ==================================================================
#define SUCCESS 0
#define ERROR_GENERIC -1

#endif