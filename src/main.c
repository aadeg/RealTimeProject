/*
 * main.c
 * 
 */

#include <stdbool.h>
#include <allegro.h>
#include <pthread.h>
#include <math.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

#include "ptask.h"
#include "graphics.h"
#include "consts.h"
#include "structs.h"


// ==================================================================
//                        ERROR MESSAGES
// ==================================================================
#define ERR_MSG_TASK_CREATE 	"Error while creating %s. Errno %d\n"
#define ERR_MSG_TASK_CREATE_AIR "Error while creating airplane task %d. Errno %d\n"
#define ERR_MSG_TASK_JOIN   	"Error while joining %s. Errno %d\n"
#define ERR_MSG_TASK_JOIN_AIR   "Error while joining airplane task %d. Errno %d\n"
#define ERR_MSG_TASK_AIR_DM		"Airplane task %02d - deadline missed\n"


// ==================================================================
//                        GLOBAL VARIABLES
// ==================================================================
trajectory_t holding_trajectory;
trajectory_t terminal_trajectory;
trajectory_t runway_landing_trajectories[N_RUNWAYS];
trajectory_t runway_takeoff_trajectories[N_RUNWAYS];

task_info_t airplane_task_infos[MAX_AIRPLANE];
airplane_pool_t airplane_pool;
airplane_queue_t airplane_queue;  // Serving queue
shared_system_state_t system_state;
task_state_t task_states[N_TASKS];

bool show_trails = true;
bool show_next_waypoint = false;
bool enable_random_gen = false;
bool end_all = false;			// true if the program should terminate


// ==================================================================
//                      FUNCTIONS DECLARATION
// ==================================================================
// Init functions
void init(void);
void init_holding_trajectory(void);
void init_landing_trajectories(void);
void init_takeoff_trajectories(void);
void init_terminal_trajectory(void);
void init_task_states(void);
void init_system_state(void);

// Task functions
void* graphic_task(void* arg);
void* airplane_task(void* arg);
void* traffic_controller_task(void* arg);
void* input_task(void* arg);
void* random_gen_task(void* arg);

// Task related functions
void create_tasks(task_info_t* graphic_task_info, task_info_t* input_task_info,
	task_info_t* traffic_ctrl_task_info, task_info_t* random_gen_task_info);
void join_tasks(task_info_t* graphic_task_info, task_info_t* input_task_info,
	task_info_t* traffic_ctrl_task_info, task_info_t* random_gen_task_info);

// Airplane spawning functions
void spawn_inbound_airplane(void);
void spawn_outbound_airplane(void);
void run_new_airplane(shared_airplane_t* airplane);

// Airplane control
void airplane_controller_evolve(airplane_t* airplane);
void compute_airplane_controls(const airplane_t* airplane,
	const waypoint_t* des_point, float* accel_cmd, float* omega_cmd);
void update_airplane_state(airplane_t* airplane, float accel_cmd, 
	float omega_cmd);
void update_airplane_des_trajectory(airplane_t* airplane, 
	const waypoint_t* des_point);
float wrap_angle_pi(float angle);
float points_distance(float x1, float y1, float x2, float y2);

// Traffic controller
void traffic_controller_free_runway(shared_airplane_t** runways, int runway_id);
void traffic_controller_assign_runway(shared_airplane_t** runways, int runway_id);

// Graphic task functions
void update_main_box(BITMAP* main_box, airplane_t* airplanes, cbuffer_t* trails);
int copy_shared_airplanes(airplane_t* dst, int max_size);
void update_airplane_trail(const airplane_t* airplane, cbuffer_t* trail);
void handle_trails(BITMAP* bitmap, airplane_t* airplanes, int n_airplanes,
	cbuffer_t* trails, bool show_trails);
void toggle_trails(void);
void toggle_next_waypoint(void);
void toggle_random_gen(void);

// Utility functions
void update_task_states(const task_info_t* task_info);
float linear_interpolate(float start, float end, int n, int index);
void get_random_inbound_state(float* x, float* y, float* angle);
void get_random_outbound_state(float* x, float* y, float* angle);


// ==================================================================
//                    			MAIN
// ==================================================================
int main(void) {
	task_info_t graphic_task_info;
	task_info_t input_task_info;
	task_info_t traffic_ctlr_task_info;
	task_info_t random_gen_task_info;

	init();
	create_tasks(&graphic_task_info, &input_task_info,
		&traffic_ctlr_task_info, &random_gen_task_info);
	join_tasks(&graphic_task_info, &input_task_info,
		&traffic_ctlr_task_info, &random_gen_task_info);

	// Ensure correct deallocation of the airplanes
	assert(airplane_pool.n_free == AIRPLANE_POOL_SIZE);

	allegro_exit();
	return 0;
}

// ==================================================================
//                           AIRPLANE TASK
// ==================================================================
void* airplane_task(void* arg) {
	task_info_t* task_info = (task_info_t*) arg;
	shared_airplane_t* global_airplane_ptr = (shared_airplane_t*) task_info->arg;
	// Local copy of the airplane information
	airplane_t local_airplane = global_airplane_ptr->airplane;

	task_states[task_info->task_num].is_running = true;
	task_set_activation(task_info);

	while (!end_all && !local_airplane.kill) {
		// Updating the local copy of the airplane struct
		pthread_mutex_lock(&global_airplane_ptr->mutex);
		local_airplane = global_airplane_ptr->airplane;
		pthread_mutex_unlock(&global_airplane_ptr->mutex);

		// Computing control and updating the airplane state
		airplane_controller_evolve(&local_airplane);

		// Updating the global airplane struct
		pthread_mutex_lock(&global_airplane_ptr->mutex);
		global_airplane_ptr->airplane = local_airplane;
		pthread_mutex_unlock(&global_airplane_ptr->mutex);

		// Ending task instance
		if (task_deadline_missed(task_info)) {
			fprintf(stderr, ERR_MSG_TASK_AIR_DM,task_info->task_num);
		}
		update_task_states(task_info);
		task_wait_for_activation(task_info);
	}
	
	printf("Killing airplane task %d\n", task_info->task_num);
	airplane_pool_free(&airplane_pool, global_airplane_ptr);
	pthread_mutex_lock(&system_state.mutex);
	--system_state.state.n_airplanes;
	pthread_mutex_unlock(&system_state.mutex);
	task_states[task_info->task_num].is_running = false;

	return NULL;
}


// ==================================================================
//                     TRAFFIC CONTROLLER TASK
// ==================================================================
void* traffic_controller_task(void* arg) {
	task_info_t* task_info = (task_info_t*) arg;
	// Pointers to the airplanes assigned to a runway
	shared_airplane_t* runways[N_RUNWAYS] = { 0 };
	int i = 0;

	task_states[task_info->task_num].is_running = true;
	task_set_activation(task_info);

	while (!end_all) {
		// Checking if an airplane has freed the runway and assigning the runway
		// to a new airplane
		for (i = 0; i < N_RUNWAYS; ++i) {
			traffic_controller_free_runway(runways, i);
			traffic_controller_assign_runway(runways, i);
		}

		// Updating the system state
		pthread_mutex_lock(&system_state.mutex);
		for (i = 0; i < N_RUNWAYS; ++i)
			system_state.state.is_runway_free[i] = (runways[i] == NULL);
		pthread_mutex_unlock(&system_state.mutex);

		// Ending task instance
		if (task_deadline_missed(task_info)) {
			fprintf(stderr, "Traffic controller task deadline missed\n");
		}
		update_task_states(task_info);
		task_wait_for_activation(task_info);
	}

	task_states[task_info->task_num].is_running = false;
	return NULL;
}


// ==================================================================
//                           GRAPHIC TASK
// ==================================================================
void* graphic_task(void* arg) {
	task_info_t* task_info = (task_info_t*) arg;
	BITMAP* sidebar_box = create_sidebar_box();
	BITMAP* main_box = create_main_box();
	int i = 0;

	airplane_t local_airplanes[MAX_AIRPLANE];
	cbuffer_t airplane_trails[MAX_AIRPLANE];

	for (i = 0; i < MAX_AIRPLANE; ++i)
		cbuffer_init(&airplane_trails[i]);

	task_states[task_info->task_num].is_running = true;
	task_set_activation(task_info);

	while (!end_all) {
		clear_main_box(main_box);

		// Drawing Main Box
		update_main_box(main_box, local_airplanes, airplane_trails);
		blit_main_box(main_box);

		// Drawing Status Box
		update_sidebar_box(sidebar_box, &system_state, task_states, N_TASKS);
		blit_sidebar_box(sidebar_box);

		// Ending task instance
		if (task_deadline_missed(task_info)) {
			fprintf(stderr, "Graphic task - Deadline miss\n");
		}
		update_task_states(task_info);
		task_wait_for_activation(task_info);
	}

	printf("Exiting...\n");
	task_states[task_info->task_num].is_running = false;
	destroy_bitmap(main_box);
	destroy_bitmap(sidebar_box);
  return NULL;
}


// ==================================================================
//                           INPUT TASK
// ==================================================================
void* input_task(void* arg) {
	task_info_t* task_info = (task_info_t*) arg;

	char scan = '\0';
	char ascii = '\0';
	bool got_key = false;

	task_states[task_info->task_num].is_running = true;
	task_set_activation(task_info);

	do {
		got_key =  get_keycodes(&scan, &ascii);
		if (got_key && scan == KEY_O) {
			spawn_outbound_airplane();
		} else if (got_key && scan == KEY_I) {
			spawn_inbound_airplane();
		} else if (got_key && scan == KEY_T) {
			toggle_trails();
		} else if (got_key && scan == KEY_W) {
			toggle_next_waypoint();
		} else if (got_key && scan == KEY_R) {
			toggle_random_gen();
		}

		// Ending task instance
		if (task_deadline_missed(task_info))
			fprintf(stderr, "Input task deadline missed\n");
		update_task_states(task_info);
		if (scan != KEY_ESC)
			task_wait_for_activation(task_info);
	} while (scan != KEY_ESC);

	printf("Exiting...\n");
	task_states[task_info->task_num].is_running = false;
	end_all = true;
	return NULL;
}


// ==================================================================
//                     RANDOM GENERATION TASK
// ==================================================================
void* random_gen_task(void* arg) {
	task_info_t* task_info = (task_info_t*) arg;
	int rand_number = 0;

	task_states[task_info->task_num].is_running = true;
	task_set_activation(task_info);

	while (!end_all) {
		if (enable_random_gen) {
			rand_number = rand();
			// Equal probability to spawn an inbound or an outbound airplane
			if (rand_number < RAND_MAX / 2) {
				spawn_inbound_airplane();
			} else {
				spawn_outbound_airplane();
			}
		}

		// Ending task instance
		if (task_deadline_missed(task_info)) {
			fprintf(stderr, "Traffic controller task deadline missed\n");
		}
		update_task_states(task_info);
		task_wait_for_activation(task_info);
	}

	task_states[task_info->task_num].is_running = false;
	return NULL;
}


// ==================================================================
//                      FUNCTIONS DEFINITION
// ==================================================================
void init(void) {
	// Allegro initialization
	allegro_init();
	install_keyboard();
	set_color_depth(8);
	set_gfx_mode(GFX_AUTODETECT_WINDOWED, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 0);
	clear_to_color(screen, BG_COLOR);

	// Trajectories initialization
	init_holding_trajectory();
	init_landing_trajectories();
	init_terminal_trajectory();
	init_takeoff_trajectories();

	airplane_queue_init(&airplane_queue);
	airplane_pool_init(&airplane_pool);
	init_task_states();
	init_system_state();

	srand(time(NULL));
}

// Initialize the holding trajectory
void init_holding_trajectory(void) {
	int i = 0;
	waypoint_t* point = NULL;	// Pointer to the working point
	float s = 0.0;				// Curvilinear coordinate
	const float step = (2.0f * M_PI_F/ HOLDING_TRAJECTORY_SIZE);

	holding_trajectory.size = HOLDING_TRAJECTORY_SIZE;
	holding_trajectory.is_cyclic = true;
	for (i = 0; i < HOLDING_TRAJECTORY_SIZE; ++i) {
		point = &holding_trajectory.waypoints[i];
		
		// x coordinate
		point->x = HOLDING_TRAJECTORY_RADIUS * cosf(s);
		if (s < M_PI_2_F ||s >= 3.0f * M_PI_2_F) {
			point->x += HOLDING_TRAJECTORY_ARM;
		} else {
			point->x -= HOLDING_TRAJECTORY_ARM;
		}
		point->x += HOLDING_TRAJECTORY_X;

		// y coodinate
		point->y = HOLDING_TRAJECTORY_RADIUS * sinf(s);
		point->y += HOLDING_TRAJECTORY_Y;

		// velocity
		point->vel = HOLDING_TRAJECTORY_VEL;

		s += step;
	}
}

// Initialize a runway landing trajectory by interpolating
// between the inital state and the final state
void _init_landing_trajectory(trajectory_t* traj, const int size,
		const float x_start, const float y_start,
		const float x_end, const float y_end,
		const float vel_start, const float vel_end) {
	int i = 0;

	traj->size = size;
	traj->is_cyclic = false;
	for (i = 0; i < size; ++i) {
		traj->waypoints[i].x = linear_interpolate(x_start, x_end, size, i);
		traj->waypoints[i].y = linear_interpolate(y_start, y_end, size, i);
		traj->waypoints[i].vel = linear_interpolate(vel_start, vel_end, size, i);
	}
}

// Initialize the runways landing trajectory
void init_landing_trajectories(void) {
	_init_landing_trajectory(&runway_landing_trajectories[0],
		RUNWAY_0_LANDING_TRAJ_SIZE,
		RUNWAY_0_LANDING_TRAJ_START_X, RUNWAY_0_LANDING_TRAJ_START_Y,
		RUNWAY_0_LANDING_TRAJ_END_X, RUNWAY_0_LANDING_TRAJ_END_Y,
		HOLDING_TRAJECTORY_VEL, RUNWAY_0_LANDING_TRAJ_END_VEL);
	_init_landing_trajectory(&runway_landing_trajectories[1],
		RUNWAY_1_LANDING_TRAJ_SIZE,
		RUNWAY_1_LANDING_TRAJ_START_X, RUNWAY_1_LANDING_TRAJ_START_Y,
		RUNWAY_1_LANDING_TRAJ_END_X, RUNWAY_1_LANDING_TRAJ_END_Y,
		HOLDING_TRAJECTORY_VEL, RUNWAY_1_LANDING_TRAJ_END_VEL);
}

// Initialize a takeoff trajectory with the provied points
void _init_takeoff_trajectory(trajectory_t* traj, float* xs, float* ys,
		float* vels, int size) {
	int i = 0;

	traj->is_cyclic = false;
	traj->size = size;
	for (i = 0; i < size; ++i) {
		traj->waypoints[i] = (waypoint_t) {
			.x = xs[i],
			.y = ys[i],
			.vel = vels[i]
		};
	}
}

// Initialize the takeoff trajectories
void init_takeoff_trajectories(void) {
	float runway_0_xs[] 	= RUNWAY_0_TAKEOFF_TRAJ_XS;
	float runway_0_ys[] 	= RUNWAY_0_TAKEOFF_TRAJ_YS;
	float runway_0_vels[] 	= RUNWAY_0_TAKEOFF_TRAJ_VELS;
	float runway_1_xs[] 	= RUNWAY_1_TAKEOFF_TRAJ_XS;
	float runway_1_ys[] 	= RUNWAY_1_TAKEOFF_TRAJ_YS;
	float runway_1_vels[] 	= RUNWAY_1_TAKEOFF_TRAJ_VELS;

	// RUNWAY 0
	_init_takeoff_trajectory(&runway_takeoff_trajectories[0],
		runway_0_xs, runway_0_ys, runway_0_vels, 
		RUNWAY_0_TAKEOFF_TRAJ_SIZE);

	// RUNWAY 1
	_init_takeoff_trajectory(&runway_takeoff_trajectories[1],
		runway_1_xs, runway_1_ys, runway_1_vels, 
		RUNWAY_1_TAKEOFF_TRAJ_SIZE);	
}

void init_terminal_trajectory(void) {
	terminal_trajectory.is_cyclic = true;
	terminal_trajectory.size = 1;
	terminal_trajectory.waypoints[0] = (waypoint_t) {
		.x = TERMINAL_TRAJ_X,
		.y = TERMINAL_TRAJ_Y,
		.vel = TERMINAL_TRAJ_VEL
	};
}

// Initialized the task states
void init_task_states(void) {
	int i = 0;

	// Setting the names of the tasks
	for (i = 0; i < MAX_AIRPLANE; ++i) {
		sprintf(task_states[i].str, "Airplane %02d:", i + 1);
	}
	strcpy(task_states[i].str, "Graphic:");
	strcpy(task_states[i + 1].str, "Input:");
	strcpy(task_states[i + 2].str, "Traffic Control:");
	strcpy(task_states[i + 3].str, "Random Gen.:");
}

// Initialized the system state
void init_system_state(void) {
	system_state.state = (system_state_t) {
		.is_runway_free = {true, true},
		.n_airplanes =  0,
		.random_gen_enabled = enable_random_gen
	};
	pthread_mutex_init(&system_state.mutex, NULL);
}

// Create and run the tasks
void create_tasks(task_info_t* graphic_task_info,
		task_info_t* input_task_info,
		task_info_t* traffic_ctrl_task_info,
		task_info_t* random_gen_task_info) {
	int err = 0;

	// Creating graphic task
	task_info_init(graphic_task_info, MAX_AIRPLANE, 
		GRAPHIC_PERIOD_MS, GRAPHIC_PERIOD_MS, GRAPHIC_PRIORITY);
	err = task_create(graphic_task_info, graphic_task);
	if (err) fprintf(stderr, ERR_MSG_TASK_CREATE, "graphic task", err);

	// Creating input task
	task_info_init(input_task_info, MAX_AIRPLANE + 1, 
		INPUT_PERIOD_MS, INPUT_PERIOD_MS, INPUT_PRIORITY);
	err = task_create(input_task_info, input_task);
	if (err) fprintf(stderr, ERR_MSG_TASK_CREATE, "input task", err);

	// Creating traffic controller task
	task_info_init(traffic_ctrl_task_info, MAX_AIRPLANE + 2, 
		TRAFFIC_CTRL_PERIOD_MS, TRAFFIC_CTRL_PERIOD_MS, TRAFFIC_CTRL_PRIORITY);
	err = task_create(traffic_ctrl_task_info, traffic_controller_task);
	if (err) fprintf(stderr, ERR_MSG_TASK_CREATE, "traffic controller task", err);

	// Creating random generation task
	task_info_init(random_gen_task_info, MAX_AIRPLANE + 3,
		RANDOM_GEN_PERIOD_MS, RANDOM_GEN_PERIOD_MS, RANDOM_GEN_PRIORITY);
	err = task_create(random_gen_task_info, random_gen_task);
	if (err) fprintf(stderr, ERR_MSG_TASK_CREATE, "random generation task", err);
}

// Join all the tasks
void join_tasks(task_info_t* graphic_task_info,
		task_info_t* input_task_info,
		task_info_t* traffic_ctrl_task_info,
		task_info_t* random_gen_task_info) {
	int err = 0;
	int i = 0;

	// Joining graphic task
	err = task_join(graphic_task_info, NULL);
	if (err) fprintf(stderr, ERR_MSG_TASK_JOIN, "graphic task", err);

	// Joining input task
	err = task_join(input_task_info, NULL);
	if (err) fprintf(stderr, ERR_MSG_TASK_JOIN, "input task", err);

	// Joining traffic controller task
	err = task_join(traffic_ctrl_task_info, NULL);
	if (err) fprintf(stderr, ERR_MSG_TASK_JOIN, "traffic controller", err);

	// Joining traffic controller task
	err = task_join(random_gen_task_info, NULL);
	if (err) fprintf(stderr, ERR_MSG_TASK_JOIN, "random generation", err);

	// Joining airplane tasks
	for (i = 0; i < MAX_AIRPLANE; ++i) {
		err = task_join(&airplane_task_infos[i], NULL);
		// if (err) fprintf(stderr, ERR_MSG_TASK_JOIN_AIR, i, err);
	}
}

// Initialize and spawn a new inbound airplane
void spawn_inbound_airplane(void) {
	float x = 0.0;
	float y = 0.0;
	float angle = 0.0;

	// Getting a new airplane from the pool
	shared_airplane_t* new_airplane = airplane_pool_get_new(&airplane_pool);
	if (new_airplane == NULL) return;

	// Getting a new airplane from the pool
	get_random_inbound_state(&x, &y, &angle);
	new_airplane->airplane = (airplane_t) {
		.x = x,
		.y = y,
		.angle = angle,
		.vel = HOLDING_TRAJECTORY_VEL,
		.des_traj = &holding_trajectory,
		.traj_index = 0,
		.traj_finished = false,
		.status = INBOUND_HOLDING,
		.unique_id = new_airplane->airplane.unique_id,
		.kill = false
	};
	pthread_mutex_init(&(new_airplane->mutex), NULL);

	run_new_airplane(new_airplane);
}

// Initialize and spawn a new outbound airplane
void spawn_outbound_airplane(void) {
	float x = 0;
	float y = 0;
	float angle = 0;

	// Getting a new airplane from the pool
	shared_airplane_t* new_airplane = airplane_pool_get_new(&airplane_pool);
	if (new_airplane == NULL) return;

	// Getting a new airplane from the pool
	get_random_outbound_state(&x, &y, &angle);
	new_airplane->airplane = (airplane_t) {
		.x = x,
		.y = y,
		.angle = angle,
		.vel = 0,
		.des_traj = &terminal_trajectory,
		.traj_index = 0,
		.traj_finished = false,
		.status = OUTBOUND_HOLDING,
		.unique_id = new_airplane->airplane.unique_id,
		.kill = false
	};
	pthread_mutex_init(&(new_airplane->mutex), NULL);

	run_new_airplane(new_airplane);
}

// Create and run a new task that will handle the airplane
void run_new_airplane(shared_airplane_t* airplane) {
	int airplane_id = airplane->airplane.unique_id;;
	int err = 0;

	// Updating the system state
	pthread_mutex_lock(&system_state.mutex);
	++system_state.state.n_airplanes;
	pthread_mutex_unlock(&system_state.mutex);

	// Pushing to the serving queue
	airplane_queue_push(&airplane_queue, airplane);

	// Creating and running a new task
	task_info_init(&airplane_task_infos[airplane_id], airplane_id, 
		AIRPLANE_PERIOD_MS, AIRPLANE_PERIOD_MS, AIRPLANE_PRIORITY);
	airplane_task_infos[airplane_id].arg = airplane;

	err = task_create(&airplane_task_infos[airplane_id], airplane_task);
	if (err) fprintf(stderr, "Errore while creating the task. Errno %d\n", err);
}

// Return a random float in [min, max] interval
float get_random_float(float min, float max) {
	assert(min <= max);
	float diff = max - min;
	float r = ((float) rand()) / (float) RAND_MAX;
	return r * diff + min;
}

// Return a random state for an inbound airplane
void get_random_inbound_state(float* x, float* y, float* angle) {
	*x = get_random_float(INBOUND_AREA_X, INBOUND_AREA_X + INBOUND_AREA_WIDTH);
	*y = get_random_float(INBOUND_AREA_Y, INBOUND_AREA_Y + INBOUND_AREA_HEIGHT);
	*angle = get_random_float(0, 2.0f * M_PI_F);
}

// Return a random state for an outbound airplane
void get_random_outbound_state(float* x, float* y, float* angle) {
	*x = get_random_float(OUTBOUND_AREA_X, OUTBOUND_AREA_X + OUTBOUND_AREA_WIDTH);
	*y = get_random_float(OUTBOUND_AREA_Y, OUTBOUND_AREA_Y + OUTBOUND_AREA_HEIGHT);
	*angle = get_random_float(0, 2.0f * M_PI_F);
}

// Update the main box by drawing the airplanes and the trails
void update_main_box(BITMAP* main_box, airplane_t* airplanes, cbuffer_t* trails) {
	int n_airplane = copy_shared_airplanes(airplanes, MAX_AIRPLANE);
	int i = 0;
	const airplane_t* airplane;
	const waypoint_t* des_point;
	
	// Drawing the airplane trails
	handle_trails(main_box, airplanes, n_airplane, trails, show_trails);
	
	// Drawing the airplanes
	for (i = 0; i < n_airplane; ++i) {
		airplane = &airplanes[i];
		draw_airplane(main_box, airplane);
		if (show_next_waypoint) {
			des_point = trajectory_get_point(airplane->des_traj, airplane->traj_index);
			if (des_point) draw_waypoint(main_box, des_point);
		}
	}
}

// Copy the allocated airplanes to and array.
// Return the number of the copied elements
int copy_shared_airplanes(airplane_t* dst, int max_size) {
	int i = 0;
	int n = 0;		// number of allocated airplanes
	// pointers to the allocated airplanes
	shared_airplane_t* airplanes[AIRPLANE_POOL_SIZE];

	// getting the pointers of the allocated airplanes
	pthread_mutex_lock(&airplane_pool.mutex);
	for (i = 0; i < AIRPLANE_POOL_SIZE; ++i) {
		if (!airplane_pool.is_free[i]) {
			airplanes[n] = &airplane_pool.elems[i];
			++n;
		}
	}
	pthread_mutex_unlock(&airplane_pool.mutex);

	// safe copying the airplanes to the destination array
	for (i = 0; i < n && i < max_size; ++i) {
		pthread_mutex_lock(&airplanes[i]->mutex);
		dst[i] = airplanes[i]->airplane;
		pthread_mutex_unlock(&airplanes[i]->mutex);
	}
	return n;
}

// Execute the airplane trajectory controller
void airplane_controller_evolve(airplane_t* airplane) {
	float accel_cmd = 0;				// acceleration command
	float omega_cmd = 0;				// angular rotation command
	const waypoint_t* des_point;		// pointer to the desired point

	des_point = trajectory_get_point(airplane->des_traj, airplane->traj_index);
	compute_airplane_controls(airplane, des_point, &accel_cmd, &omega_cmd);
	update_airplane_state(airplane, accel_cmd, omega_cmd);
	update_airplane_des_trajectory(airplane, des_point);
}

// Compute the airplane controls, i.e. the acceleration command and the
// angular velocity command
void compute_airplane_controls(const airplane_t* airplane, 
		const waypoint_t* des_point, float* accel_cmd, float* omega_cmd) {
	float des_angle = 0.0;			// desired angle
	float angle_error = 0.0;
	float vel_error = 0.0;
	
	if (des_point) {
		// acceleration command
		vel_error = des_point->vel - airplane->vel;
		*accel_cmd = AIRPLANE_CTRL_VEL_GAIN * vel_error;

		// angular velocity command
		des_angle = atan2f(des_point->y - airplane->y,
							des_point->x - airplane->x);
		angle_error = wrap_angle_pi(des_angle - airplane->angle);
		*omega_cmd = AIRPLANE_CTRL_OMEGA_GAIN * angle_error;
	} else {
		// backup commands in the cases whene des_point is NULL
		*omega_cmd = 0;
		*accel_cmd = 0;
	}
}

// Update the airplane state given the commands from the controller
void update_airplane_state(airplane_t* airplane, float accel_cmd, 
		float omega_cmd) {
	float vel = airplane->vel;

	// "steering" is possible only when the airplane is moving
	if (vel < AIRPLANE_CTRL_VEL_TH) omega_cmd = 0;

	// updating the state using the unicycle model
	airplane->x += vel * cosf(airplane->angle) * AIRPLANE_CTRL_SIM_PERIOD;
	airplane->y += vel * sinf(airplane->angle) * AIRPLANE_CTRL_SIM_PERIOD;
	airplane->angle += wrap_angle_pi(omega_cmd * AIRPLANE_CTRL_SIM_PERIOD);
	airplane->vel += accel_cmd * AIRPLANE_CTRL_SIM_PERIOD;
}

// Update the desired point if the airplane has reached
// the current desired point
void update_airplane_des_trajectory(airplane_t* airplane,
		const waypoint_t* des_point) {
	float distance = 0.0;		// distance from the desired point
	float min_dist = 0.0;		// threashold distance

	// setting the min_dist based on the airplane state
	if (airplane->status == OUTBOUND_TAKEOFF)
		min_dist = AIRPLANE_CTRL_TAXI_MIN_DIST;
	else
		min_dist = AIRPLANE_CTRL_MIN_DIST;

	if (des_point) {
		// computing the distance and check the switching condition
		distance = points_distance(airplane->x, airplane->y,
			des_point->x, des_point->y);
		
		if (distance < min_dist) ++airplane->traj_index;
	} else {
		// if des_point is NULL, the airplane has reached the end of
		// the desired trajectory
		airplane->traj_finished = true;
	}
}

// Check if the airplane has freed the runway
void traffic_controller_free_runway(shared_airplane_t** runways, int runway_id) {
	shared_airplane_t* airplane = runways[runway_id];
	
	if (airplane != NULL) {
		// the runways is occupied by an airplane
		pthread_mutex_lock(&airplane->mutex);
		if (airplane->airplane.traj_finished) {
			// the airplane has reached the end of its desired trajectory
			// then the runway can be freed and the airplane can be despawned
			airplane->airplane.kill = true;
			runways[runway_id] = NULL;
		}
		pthread_mutex_unlock(&airplane->mutex);
	}
}

// Check if a runway is available. If so a new airplane will be assign to it
void traffic_controller_assign_runway(shared_airplane_t** runways, int runway_id) {	
	shared_airplane_t* airplane = runways[runway_id];
	bool is_free = false;			// hold the state of the runway

	if (airplane == NULL) {
		// the runway is free
		is_free = true;
		// getting a new airplane from the queue
		airplane = airplane_queue_pop(&airplane_queue);
	}
	
	if (is_free && airplane != NULL) {
		// the runway is free and an airplane has been retrive from the queue
		// the airplane is assign to the runway
		runways[runway_id] = airplane;
		pthread_mutex_lock(&airplane->mutex);
		if (airplane->airplane.status == INBOUND_HOLDING) {
			airplane->airplane.status = INBOUND_LANDING;
			airplane->airplane.des_traj = &runway_landing_trajectories[runway_id];
			airplane->airplane.traj_index = 0;
			airplane->airplane.traj_finished = false;
			printf("RUNWAY %d to %d\n", runway_id, airplane->airplane.unique_id);
		} else if (airplane->airplane.status == OUTBOUND_HOLDING) {
			airplane->airplane.status = OUTBOUND_TAKEOFF;
			airplane->airplane.des_traj = &runway_takeoff_trajectories[runway_id];
			airplane->airplane.traj_index = 0;
			airplane->airplane.traj_finished = false;
			printf("RUNWAY %d to %d\n", runway_id, airplane->airplane.unique_id);
		} else { 
			fprintf(stderr, "Errore status aereo: %d\n", airplane->airplane.status);
		}
		pthread_mutex_unlock(&airplane->mutex);
	}

}

// Return the distance between (x1, y1) and (x2, y2)
float points_distance(float x1, float y1, float x2, float y2) {
	float dx = x1 - x2;
	float dy = y1 - y2;
	return sqrtf(dx*dx + dy*dy);
}

// Return the provided angle wrapped in [-pi, pi]
float wrap_angle_pi(float angle) {
	float k = ceilf(-angle / (2.0f * M_PI_F) - 0.5f);
	return angle + 2.0f * M_PI_F * k;
}

// Append the current position of the airplane to the trail buffer
void update_airplane_trail(const airplane_t* airplane, cbuffer_t* trail) {
	int i = 0;
	point2i_t new_point;
	
	// converting the cartesian coordinates to display coordinates
	convert_coord_to_display(airplane->x, airplane->y,
		&new_point.x, &new_point.y);
	
	// appending the new point
	i = cbuffer_next_index(trail);
	trail->points[i] = new_point;
}

// handle the update, the draw and the reset of the trail array
void handle_trails(BITMAP* bitmap, airplane_t* airplanes, int n_airplanes,
		cbuffer_t* trails, bool show_trails_) {
	int i = 0;
	airplane_t* airplane = NULL;
	cbuffer_t* trail = NULL;
	// true if the corresponding trail has been updated
	// used to determined which trails should be resetted
	bool trails_updated[MAX_AIRPLANE] = { 0 };

	// updating and drawing of the trails
	for (i = 0; i < n_airplanes; ++i) {
		airplane = &airplanes[i];
		trail = &trails[airplane->unique_id];
		trails_updated[airplane->unique_id] = true;
		update_airplane_trail(airplane, trail);
		if (show_trails_)
			draw_trail(bitmap, trail, TRAIL_COLOR);
	}

	// resetting the unused trail buffers
	for (i = 0; i < MAX_AIRPLANE; ++i) {
		if (trails_updated[i] == false)
			cbuffer_init(&trails[i]);
	}
}

void toggle_trails(void) {
	show_trails = !show_trails;
}

void toggle_next_waypoint(void) {
	show_next_waypoint = !show_next_waypoint;
}

void toggle_random_gen(void) {
	enable_random_gen = !enable_random_gen;
	
	// Updating the system state
	pthread_mutex_lock(&system_state.mutex);
	system_state.state.random_gen_enabled = enable_random_gen;
	pthread_mutex_unlock(&system_state.mutex);
}

void update_task_states(const task_info_t* task_info) {
	task_states[task_info->task_num].deadline_miss = task_info->deadline_miss;
}

// Return the i-th element of a linear interpolation made 
// from "start" to "end" in "n" steps
float linear_interpolate(float start, float end, int n, int i) {
	assert(i < n);
	const float step_size = (end - start) / (float) n;

	return start + step_size * (float) i;
}