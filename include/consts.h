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

// Sidebar Box
#define SIDEBAR_BOX_WIDTH		(SCREEN_WIDTH - SCREEN_HEIGHT - 20)
#define SIDEBAR_BOX_HEIGHT		(SCREEN_HEIGHT - 20)
#define SIDEBAR_BOX_X			(SCREEN_HEIGHT + 10)
#define SIDEBAR_BOX_Y			10
#define SIDEBAR_BOX_PADDING 	7
#define SIDEBAR_BOX_VSPACE		12
#define SIDEBAR_BOX_FIRST_COL_X	100	

// Airplane
#define AIRPLANE_SIZE			10
#define AIRPLANE_COLOR			15
#define AIRPLANE_RUNWAY_COLOR	12
#define AIRPLANE_TRAIL_COLOR	10
#define TRAIL_COLOR				2
#define WAYPOINT_RADIUS			2
#define WAYPOINT_COLOR			4

// ==================================================================
//                     AIRPLANE TASK CONSTANTS
// ==================================================================
#define AIRPLANE_CTRL_OMEGA_GAIN	2.0f
#define AIRPLANE_CTRL_VEL_GAIN		1.0f
#define AIRPLANE_CTRL_MIN_DIST		20.0f
#define AIRPLANE_CTRL_TAXI_MIN_DIST	5.0f
#define AIRPLANE_CTRL_VEL			15.0f
#define AIRPLANE_CTRL_SIM_PERIOD	0.020f
#define AIRPLANE_CTRL_VEL_TH		0.01f

// ==================================================================
//                     TRAJECTORY CONSTANTS
// ==================================================================
#define HOLDING_TRAJECTORY_SIZE 		30
#define HOLDING_TRAJECTORY_RADIUS 		60.0f
#define HOLDING_TRAJECTORY_ARM			100.0f
#define HOLDING_TRAJECTORY_X			-180.0f
#define HOLDING_TRAJECTORY_Y			180.0f
#define HOLDING_TRAJECTORY_VEL			50.0f

#define RUNWAY_0_LANDING_TRAJ_SIZE		15
#define RUNWAY_0_LANDING_TRAJ_START_X	-330.0f
#define RUNWAY_0_LANDING_TRAJ_START_Y	-140.0f
#define RUNWAY_0_LANDING_TRAJ_END_X		100.0f
#define RUNWAY_0_LANDING_TRAJ_END_Y		-140.0f
#define RUNWAY_0_LANDING_TRAJ_END_VEL	5.0f

#define RUNWAY_1_LANDING_TRAJ_SIZE		10
#define RUNWAY_1_LANDING_TRAJ_START_X	-330.0f
#define RUNWAY_1_LANDING_TRAJ_START_Y	-80.0f
#define RUNWAY_1_LANDING_TRAJ_END_X		100.0f
#define RUNWAY_1_LANDING_TRAJ_END_Y		-80.0f
#define RUNWAY_1_LANDING_TRAJ_END_VEL	5.0f

#define TERMINAL_TRAJ_X		-190
#define TERMINAL_TRAJ_Y		-238
#define TERMINAL_TRAJ_VEL	0

#define TAXI_TRAJ_VEL		10.0f
#define TAKEOFF_TRAJ_VEL	50.0f
#define RUNWAY_0_TAKEOFF_TRAJ_SIZE		12
#define RUNWAY_0_TAKEOFF_TRAJ_XS		{TERMINAL_TRAJ_X, -190.0f, -165.0f, -140.0f, -110.0f, -110.0f, -100.0f,  -70.0f,    0.0f, 100.0f,  280.0f,  350.0f}
#define RUNWAY_0_TAKEOFF_TRAJ_YS		{TERMINAL_TRAJ_Y, -220.0f, -190.0f, -190.0f, -160.0f, -150.0f, -140.0f, -140.0f, -140.0f, -140.0f, -140.0f, -350.0f}
#define RUNWAY_0_TAKEOFF_TRAJ_VELS		{TAXI_TRAJ_VEL, TAXI_TRAJ_VEL, TAXI_TRAJ_VEL, TAXI_TRAJ_VEL, TAXI_TRAJ_VEL, TAXI_TRAJ_VEL, TAXI_TRAJ_VEL, TAXI_TRAJ_VEL, TAKEOFF_TRAJ_VEL, TAKEOFF_TRAJ_VEL, TAKEOFF_TRAJ_VEL, TAKEOFF_TRAJ_VEL}

#define RUNWAY_1_TAKEOFF_TRAJ_SIZE		14
#define RUNWAY_1_TAKEOFF_TRAJ_XS		{TERMINAL_TRAJ_X, -190.0f, -165.0f, -160.0f, -160.0f, -145.0f, -125.0f, -110.0f, -100.0f, -70.0f,   0.0f, 100.0f,  280.0f, 350.0f}
#define RUNWAY_1_TAKEOFF_TRAJ_YS		{TERMINAL_TRAJ_Y, -220.0f, -190.0f, -180.0f, -125.0f, -110.0f, -110.0f,  -95.0f,  -80.0f, -80.0f, -80.0f, -80.0f, -80.0f, 100.0f}
#define RUNWAY_1_TAKEOFF_TRAJ_VELS		{TAXI_TRAJ_VEL, TAXI_TRAJ_VEL, TAXI_TRAJ_VEL, TAXI_TRAJ_VEL, TAXI_TRAJ_VEL, TAXI_TRAJ_VEL, TAXI_TRAJ_VEL, TAXI_TRAJ_VEL, TAXI_TRAJ_VEL, TAXI_TRAJ_VEL, TAKEOFF_TRAJ_VEL, TAKEOFF_TRAJ_VEL, TAKEOFF_TRAJ_VEL, TAKEOFF_TRAJ_VEL}


// ==================================================================
//               AIR TRAFFIC CONTROLLER TASK CONSTANTS
// ==================================================================
#define N_RUNWAYS		2


// ==================================================================
//                     SPAWING AREA CONSTANTS
// ==================================================================
#define INBOUND_AREA_WIDTH		350.0f
#define INBOUND_AREA_HEIGHT		350.0f
#define INBOUND_AREA_X			-20.0f
#define INBOUND_AREA_Y			0.0f

#define OUTBOUND_AREA_WIDTH		210.0f
#define OUTBOUND_AREA_HEIGHT	50.0f
#define OUTBOUND_AREA_X			-355.0f
#define OUTBOUND_AREA_Y			-305.0f


// ==================================================================
//                     ARRAYS MAX LENGTH
// ==================================================================
#define MAX_AIRPLANE			30
#define AIRPLANE_POOL_SIZE		MAX_AIRPLANE
#define N_TASKS					(MAX_AIRPLANE + 4)
#define TRAIL_BUFFER_LENGTH		50
#define MAX_WAYPOINTS 			50
#define AIRPLANE_QUEUE_LENGTH	(MAX_AIRPLANE + 1)
#define TASK_NAME_LENGTH		30
#define SIDEBAR_STR_LENGTH		40

// ==================================================================
//                     SCHEDULING CONSTANTS
// ==================================================================
#define GRAPHIC_PERIOD_MS		30
#define INPUT_PERIOD_MS			30
#define AIRPLANE_PERIOD_MS 		20
#define TRAFFIC_CTRL_PERIOD_MS	5
#define RANDOM_GEN_PERIOD_MS	2000
#define PRIORITY      			50

// ==================================================================
//                     		UTILITIES
// ==================================================================
#define SUCCESS 0
#define ERROR_GENERIC -1

// ==================================================================
//                            MATH CONSTANTS
// ==================================================================
// Redefinition of float math constants
#define M_PI_F		3.14159265358979323846f
#define M_PI_2_F	1.57079632679489661923f

#endif