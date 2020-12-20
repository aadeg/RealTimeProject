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

// Status Box
#define STATUS_BOX_WIDTH		SCREEN_WIDTH - SCREEN_HEIGHT - 20
#define STATUS_BOX_HEIGHT		SCREEN_HEIGHT - 20
#define STATUS_BOX_X			SCREEN_HEIGHT + 10
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
#define AIRPLANE_CTRL_MIN_DIST		20.0
#define AIRPLANE_CTRL_VEL			15.0
#define AIRPLANE_CTRL_SIM_PERIOD	0.020

// ==================================================================
//                     TRAJECTORY CONSTATNS
// ==================================================================
#define HOLDING_TRAJECTORY_SIZE 		30
#define HOLDING_TRAJECTORY_STEP 		2.0 * M_PI / HOLDING_TRAJECTORY_SIZE
#define HOLDING_TRAJECTORY_RADIUS 		70
#define HOLDING_TRAJECTORY_ARM			100

// ==================================================================
//                     ARRAYS MAX LENGTH
// ==================================================================
#define MAX_AIRPLANE			10
#define TRAIL_BUFFER_LENGTH		100
#define MAX_WAYPOINTS 			50

// ==================================================================
//                     SCHEDULING CONSTANTS
// ==================================================================
#define GRAPHIC_PERIOD_MS		30
#define INPUT_PERIOD_MS			30
#define AIRPLANE_PERIOD_MS 		20
#define PRIORITY      			50

#endif