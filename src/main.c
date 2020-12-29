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
bool end = false;			// true if the program should terminate


// ==================================================================
//                      FUNCTIONS DECLARATION
// ==================================================================
void init();
void init_holding_trajectory();
void init_runway_trajectories();
void init_takeoff_trajectories();
void init_terminal_trajectory();
void init_task_states();
float linear_interpolate(float start, float end, int n, int index);
void create_tasks(task_info_t* graphic_task_info, task_info_t* input_task_info,
	task_info_t* traffic_ctrl_task_info);
void join_tasks(task_info_t* graphic_task_info, task_info_t* input_task_info,
	task_info_t* traffic_ctrl_task_info);
void spawn_inbound_airplane();
void spawn_outbound_airplane();
void run_new_airplane(shared_airplane_t* airplane);
int update_local_airplanes(airplane_t* dst, int max_size);
void update_airplane_trail(const airplane_t* airplane, cbuffer_t* trail);
void init_trail_buffer(cbuffer_t* trail);
void handle_trails(BITMAP* bitmap, airplane_t* airplanes, int n_airplanes,
	cbuffer_t* trails, bool show_trails);
void toggle_trails();
void toggle_next_waypoint();
void update_task_states(const task_info_t* task_info);


const waypoint_t* trajectory_get_point(const trajectory_t* trajectory, int index);
int cbuffer_next_index(cbuffer_t* buffer);
void get_random_inbound_state(float* x, float* y, float* angle);
void get_random_outbound_state(float* x, float* y, float* angle);


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
int airplane_queue_copy(airplane_queue_t* queue,
	airplane_t* dst_array, int array_size);
void airplane_queue_print(airplane_queue_t* queue);

// Airplane control
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

// Task functions
void* graphic_task(void* arg);
void* airplane_task(void* arg);
void* traffic_controller_task(void* arg);
void* input_task(void* arg);


// ==================================================================
//                    			MAIN
// ==================================================================
int main() {
	task_info_t graphic_task_info;
	task_info_t input_task_info;
	task_info_t traffic_ctlr_task_info;

	init();
	create_tasks(&graphic_task_info, &input_task_info, &traffic_ctlr_task_info);
	join_tasks(&graphic_task_info, &input_task_info, &traffic_ctlr_task_info);

	assert(airplane_pool.n_free == AIRPLANE_POOL_SIZE);

	allegro_exit();
	return 0;
}


// ==================================================================
//                           GRAPHIC TASK
// ==================================================================
void* graphic_task(void* arg) {
	struct task_info* task_info = (struct task_info*) arg;
	int counter = 0;
	int i = 0;
	const waypoint_t* des_point;
	BITMAP* sidebar_box = create_sidebar_box();
	BITMAP* main_box = create_main_box();

	int local_n_airplanes = 0;
	airplane_t local_airplanes[MAX_AIRPLANE];
	cbuffer_t airplane_trails[MAX_AIRPLANE];
	airplane_t* airplane = NULL;

	for (i = 0; i < MAX_AIRPLANE; ++i)
		init_trail_buffer(&airplane_trails[i]);

	task_states[task_info->task_num].is_running = true;
	task_set_activation(task_info);

	while (!end) {
		clear_main_box(main_box);

		// Drawing the airplane trails
		local_n_airplanes = update_local_airplanes(local_airplanes, MAX_AIRPLANE);
		handle_trails(main_box, local_airplanes, local_n_airplanes,
			airplane_trails, show_trails);
		
		// Drawing the airplanes
		for (i = 0; i < local_n_airplanes; ++i) {
			airplane = &local_airplanes[i];
			draw_airplane(main_box, airplane);
			if (show_next_waypoint) {
				des_point = trajectory_get_point(airplane->des_traj, airplane->traj_index);
				if (des_point) draw_point(main_box, des_point, 4);
			}
		}

		// Drawing Main Box
		blit_main_box(main_box);

		// Drawing Status Box
		update_sidebar_box(sidebar_box, &system_state, task_states, N_TASKS);
		blit_sidebar_box(sidebar_box);
		
		++counter;

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
//                           AIRPLANE TASK
// ==================================================================
void* airplane_task(void* arg) {
	struct task_info* task_info = (struct task_info*) arg;
	shared_airplane_t* global_airplane_ptr = (shared_airplane_t*) task_info->arg;
	float accel_cmd = 0;
	float omega_cmd = 0;
	const waypoint_t* des_point;
	airplane_t local_airplane = global_airplane_ptr->airplane;

	task_states[task_info->task_num].is_running = true;
	task_set_activation(task_info);

	while (!end & !local_airplane.kill) {
		// Updating the local copy of the airplane struct
		pthread_mutex_lock(&global_airplane_ptr->mutex);
		local_airplane = global_airplane_ptr->airplane;
		pthread_mutex_unlock(&global_airplane_ptr->mutex);

		// Computing control; updating the state; updating the reference point
		des_point = trajectory_get_point(
			local_airplane.des_traj, local_airplane.traj_index);
		compute_airplane_controls(&local_airplane, des_point, &accel_cmd, &omega_cmd);
		update_airplane_state(&local_airplane, accel_cmd, omega_cmd);
		update_airplane_des_trajectory(&local_airplane, des_point);

		// Updating the global airplane struct
		pthread_mutex_lock(&global_airplane_ptr->mutex);
		global_airplane_ptr->airplane = local_airplane;
		pthread_mutex_unlock(&global_airplane_ptr->mutex);

		// Ending task instance
		if (task_deadline_missed(task_info)) {
			fprintf(stderr, "Airplane task deadline missed\n");
		}
		update_task_states(task_info);
		task_wait_for_activation(task_info);
	}
	printf("Killing airplane task %d\n", local_airplane.unique_id);
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
	shared_airplane_t* runways[N_RUNWAYS] = { 0 };
	int i = 0;

	task_states[task_info->task_num].is_running = true;
	task_set_activation(task_info);

	while (!end) {
		for (i = 0; i < N_RUNWAYS; ++i) {
			traffic_controller_free_runway(runways, i);
			traffic_controller_assign_runway(runways, i);
		}

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
			printf("Key O pressed\n");
			spawn_outbound_airplane();
		} else if (got_key && scan == KEY_I) {
			printf("Key I pressed\n");
			spawn_inbound_airplane();
		} else if (got_key && scan == KEY_T) {
			toggle_trails();
		} else if (got_key && scan == KEY_W) {
			toggle_next_waypoint();
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
	end = true;
	return NULL;
}


// ==================================================================
//                      FUNCTIONS DEFINITION
// ==================================================================
void init() {
	allegro_init();
	install_keyboard();

	set_color_depth(8);
	set_gfx_mode(GFX_AUTODETECT_WINDOWED, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 0);
	clear_to_color(screen, BG_COLOR);

	init_holding_trajectory();
	init_runway_trajectories();
	init_terminal_trajectory();
	init_takeoff_trajectories();
	airplane_queue_init(&airplane_queue);
	airplane_pool_init(&airplane_pool);
	pthread_mutex_init(&system_state.mutex, NULL);
	init_task_states();

	srand(time(NULL));
}

void init_holding_trajectory() {
	int i = 0;
	float s = 0.0;		// Curvilinear coordinate
	waypoint_t* point;

	holding_trajectory.size = HOLDING_TRAJECTORY_SIZE;
	holding_trajectory.is_cyclic = true;
	for (i = 0; i < HOLDING_TRAJECTORY_SIZE; ++i) {
		point = &holding_trajectory.waypoints[i];
		
		point->x = HOLDING_TRAJECTORY_RADIUS * cosf(s);
		if (s < M_PI_2 ||s >= 3.0 * M_PI_2) {
			point->x += HOLDING_TRAJECTORY_ARM;
		} else {
			point->x -= HOLDING_TRAJECTORY_ARM;
		}

		point->y = HOLDING_TRAJECTORY_RADIUS * sinf(s);
		point->x += HOLDING_TRAJECTORY_X;
		point->y += HOLDING_TRAJECTORY_Y;
		point->angle = s + M_PI;
		point->vel = HOLDING_TRAJECTORY_VEL;
		s += HOLDING_TRAJECTORY_STEP;
	}
}

float linear_interpolate(float start, float end, int n, int index) {
	assert(index < n);
	const float step = 1.0 / n;

	return start + step * index * (end - start);
}

void init_runway_trajectory(trajectory_t* traj, const int size,
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

void init_runway_trajectories() {
	init_runway_trajectory(&runway_landing_trajectories[0],
		RUNWAY_0_LANDING_TRAJ_SIZE,
		RUNWAY_0_LANDING_TRAJ_START_X, RUNWAY_0_LANDING_TRAJ_START_Y,
		RUNWAY_0_LANDING_TRAJ_END_X, RUNWAY_0_LANDING_TRAJ_END_Y,
		HOLDING_TRAJECTORY_VEL, RUNWAY_0_LANDING_TRAJ_END_VEL);
	init_runway_trajectory(&runway_landing_trajectories[1],
		RUNWAY_1_LANDING_TRAJ_SIZE,
		RUNWAY_1_LANDING_TRAJ_START_X, RUNWAY_1_LANDING_TRAJ_START_Y,
		RUNWAY_1_LANDING_TRAJ_END_X, RUNWAY_1_LANDING_TRAJ_END_Y,
		HOLDING_TRAJECTORY_VEL, RUNWAY_1_LANDING_TRAJ_END_VEL);
}

void init_takeoff_trajectories() {
	int i = 0;
	float runway_0_xs[] 	= RUNWAY_0_TAKEOFF_TRAJ_XS;
	float runway_0_ys[] 	= RUNWAY_0_TAKEOFF_TRAJ_YS;
	float runway_0_vels[] 	= RUNWAY_0_TAKEOFF_TRAJ_VELS;
	float runway_1_xs[] 	= RUNWAY_1_TAKEOFF_TRAJ_XS;
	float runway_1_ys[] 	= RUNWAY_1_TAKEOFF_TRAJ_YS;
	float runway_1_vels[] 	= RUNWAY_1_TAKEOFF_TRAJ_VELS;

	trajectory_t* traj = &runway_takeoff_trajectories[0];
	traj->is_cyclic = false;
	traj->size = RUNWAY_0_TAKEOFF_TRAJ_SIZE;

	for (i = 0; i < RUNWAY_0_TAKEOFF_TRAJ_SIZE; ++i) {
		traj->waypoints[i] = (waypoint_t) {
			.x = runway_0_xs[i],
			.y = runway_0_ys[i],
			.vel = runway_0_vels[i]
		};
	}

	traj = &runway_takeoff_trajectories[1];
	traj->is_cyclic = false;
	traj->size = RUNWAY_1_TAKEOFF_TRAJ_SIZE;
	for (i = 0; i < RUNWAY_1_TAKEOFF_TRAJ_SIZE; ++i) {
		traj->waypoints[i] = (waypoint_t) {
			.x = runway_1_xs[i],
			.y = runway_1_ys[i],
			.vel = runway_1_vels[i]
		};
	}
	
}

void init_terminal_trajectory() {
	terminal_trajectory.is_cyclic = true;
	terminal_trajectory.size = 1;
	terminal_trajectory.waypoints[0] = (waypoint_t) {
		.x = TERMINAL_TRAJ_X,
		.y = TERMINAL_TRAJ_Y,
		.vel = TERMINAL_TRAJ_VEL
	};
}

void init_task_states() {
	int i = 0;

	for (i = 0; i < MAX_AIRPLANE; ++i) {
		sprintf(task_states[i].str, "Airplane %02d:", i + 1);
	}
	strcpy(task_states[i].str, "Graphic:");
	strcpy(task_states[i + 1].str, "Input:");
	strcpy(task_states[i + 2].str, "Traffic Control:");
		
}

void create_tasks(task_info_t* graphic_task_info, task_info_t* input_task_info,
		task_info_t* traffic_ctrl_task_info) {
	int err = 0;

	// Creating graphic task
	task_info_init(graphic_task_info, MAX_AIRPLANE, 
		GRAPHIC_PERIOD_MS, GRAPHIC_PERIOD_MS, PRIORITY);
	err = task_create(graphic_task_info, graphic_task);
	if (err) fprintf(stderr, ERR_MSG_TASK_CREATE, "graphic task", err);

	// Creating input task
	task_info_init(input_task_info, MAX_AIRPLANE + 1, 
		INPUT_PERIOD_MS, INPUT_PERIOD_MS, PRIORITY);
	err = task_create(input_task_info, input_task);
	if (err) fprintf(stderr, ERR_MSG_TASK_CREATE, "input task", err);

	// Creating traffic controller task
	task_info_init(traffic_ctrl_task_info, MAX_AIRPLANE + 2, 
		TRAFFIC_CTRL_PERIOD_MS, TRAFFIC_CTRL_PERIOD_MS, PRIORITY - 2);
	err = task_create(traffic_ctrl_task_info, traffic_controller_task);
	if (err) fprintf(stderr, ERR_MSG_TASK_CREATE, "traffic controller task", err);
}

void join_tasks(task_info_t* graphic_task_info, task_info_t* input_task_info,
		task_info_t* traffic_ctrl_task_info) {
	int err = 0;
	int i = 0;

	// Joining graphic task
	err = task_join(graphic_task_info, NULL);
	if (err) fprintf(stderr, ERR_MSG_TASK_JOIN, "graphic task", err);

	// Joining input task
	err = task_join(input_task_info, NULL);
	if (err) fprintf(stderr, ERR_MSG_TASK_JOIN, "input task", err);

	// Joining airplane tasks
	for (i = 0; i < MAX_AIRPLANE; ++i) {
		err = task_join(&airplane_task_infos[i], NULL);
		if (err) fprintf(stderr, ERR_MSG_TASK_JOIN_AIR, i, err);
	}
}

void spawn_inbound_airplane() {
	float x = 0.0;
	float y = 0.0;
	float angle = 0.0;

	shared_airplane_t* new_airplane = airplane_pool_get_new(&airplane_pool);
	if (new_airplane == NULL) return;

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
	run_new_airplane(new_airplane);
}

void spawn_outbound_airplane() {
	float x = 0;
	float y = 0;
	float angle = 0;

	shared_airplane_t* new_airplane = airplane_pool_get_new(&airplane_pool);
	if (new_airplane == NULL) return;

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
	run_new_airplane(new_airplane);
}

void run_new_airplane(shared_airplane_t* airplane) {
	int i = 0;
	int err = 0;

	pthread_mutex_lock(&system_state.mutex);
	++system_state.state.n_airplanes;
	pthread_mutex_unlock(&system_state.mutex);

	pthread_mutex_init(&(airplane->mutex), NULL);
	airplane_queue_push(&airplane_queue, airplane);

	i = airplane->airplane.unique_id;
	task_info_init(&airplane_task_infos[i], i, 
		AIRPLANE_PERIOD_MS, AIRPLANE_PERIOD_MS, PRIORITY - 1);
	airplane_task_infos[i].arg = airplane;

	err = task_create(&airplane_task_infos[i], airplane_task);
	if (err) fprintf(stderr, "Errore while creating the task. Errno %d\n", err);
}

float get_random_float(float min, float max) {
	assert(min <= max);
	float diff = max - min;
	float r = ((float) rand()) / RAND_MAX;
	return r * diff + min;
}

void get_random_inbound_state(float* x, float* y, float* angle) {
	*x = get_random_float(INBOUND_AREA_X, INBOUND_AREA_X + INBOUND_AREA_WIDTH);
	*y = get_random_float(INBOUND_AREA_Y, INBOUND_AREA_Y + INBOUND_AREA_HEIGHT);
	*angle = get_random_float(0, 2 * M_PI);
}

void get_random_outbound_state(float* x, float* y, float* angle) {
	*x = get_random_float(OUTBOUND_AREA_X, OUTBOUND_AREA_X + OUTBOUND_AREA_WIDTH);
	*y = get_random_float(OUTBOUND_AREA_Y, OUTBOUND_AREA_Y + OUTBOUND_AREA_HEIGHT);
	*angle = get_random_float(0, 2 * M_PI);
}

int update_local_airplanes(airplane_t* dst, int max_size) {
	assert(max_size > N_RUNWAYS);
	int i = 0;
	int j = 0;
	int n = 0;
	shared_airplane_t* airplanes[AIRPLANE_POOL_SIZE];

	pthread_mutex_lock(&airplane_pool.mutex);
	for (i = 0; i < AIRPLANE_POOL_SIZE; ++i) {
		if (!airplane_pool.is_free[i]) {
			airplanes[j] = &airplane_pool.elems[i];
			++j;
		}
	}
	pthread_mutex_unlock(&airplane_pool.mutex);
	n = j;

	for (i = 0; i < n; ++i) {
		pthread_mutex_lock(&airplanes[i]->mutex);
		dst[i] = airplanes[i]->airplane;
		pthread_mutex_unlock(&airplanes[i]->mutex);
	}
	return n;
}

void compute_airplane_controls(const airplane_t* airplane, 
		const waypoint_t* des_point, float* accel_cmd, float* omega_cmd) {
	double des_angle = 0.0;
	double angle_error = 0.0;
	double vel_error = 0.0;
	
	if (des_point) {
		des_angle = atan2f(des_point->y - airplane->y,
							des_point->x - airplane->x);
		angle_error = wrap_angle_pi(des_angle - airplane->angle);
		*omega_cmd = AIRPLANE_CTRL_OMEGA_GAIN * angle_error;
		vel_error = des_point->vel - airplane->vel;
		*accel_cmd = AIRPLANE_CTRL_VEL_GAIN * vel_error;
	} else {
		*omega_cmd = 0;
		*accel_cmd = 0;
	}
}

void update_airplane_state(airplane_t* airplane, float accel_cmd, 
		float omega_cmd) {
	float vel = airplane->vel;
	if (vel < AIRPLANE_CTRL_VEL_TH) omega_cmd = 0;

	airplane->x += vel * cosf(airplane->angle) * AIRPLANE_CTRL_SIM_PERIOD;
	airplane->y += vel * sinf(airplane->angle) * AIRPLANE_CTRL_SIM_PERIOD;
	airplane->angle += wrap_angle_pi(omega_cmd * AIRPLANE_CTRL_SIM_PERIOD);
	airplane->vel += accel_cmd * AIRPLANE_CTRL_SIM_PERIOD;
}

void update_airplane_des_trajectory(airplane_t* airplane,
		const waypoint_t* des_point) {
	float distance = 0.0;
	float min_dist = 0.0;
	if (airplane->status == OUTBOUND_TAKEOFF)
		min_dist = AIRPLANE_CTRL_TAXI_MIN_DIST;
	else
		min_dist = AIRPLANE_CTRL_MIN_DIST;

	if (des_point) {
		distance = points_distance(airplane->x, airplane->y,
			des_point->x, des_point->y);
		
		if (distance < min_dist) ++airplane->traj_index;
	} else {
		airplane->traj_finished = true;
	}
}

// Traiffic controller
void traffic_controller_free_runway(shared_airplane_t** runways, int runway_id) {
	shared_airplane_t* airplane = NULL;
	pthread_mutex_t* mutex = NULL;
	
	airplane = runways[runway_id];
	
	if (airplane != NULL) {
		mutex = &airplane->mutex;
		pthread_mutex_lock(mutex);
		if (airplane->airplane.traj_finished) {
			airplane->airplane.kill = true;
			airplane = NULL;
		}
		pthread_mutex_unlock(mutex);
	}

	runways[runway_id] = airplane;
}

void traffic_controller_assign_runway(shared_airplane_t** runways, int runway_id) {
	shared_airplane_t* airplane = NULL;
	pthread_mutex_t* mutex = NULL;
	bool is_free = false;

	airplane = runways[runway_id];
	if (airplane == NULL) {
		is_free = true;
		airplane = airplane_queue_pop(&airplane_queue);
	}

	
	if (is_free && airplane != NULL) {
		mutex = &airplane->mutex;
		pthread_mutex_lock(mutex);
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
		pthread_mutex_unlock(mutex);
	}

	runways[runway_id] = airplane;
}

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

int cbuffer_next_index(cbuffer_t* buffer) {
	int index = (buffer->top + 1) % TRAIL_BUFFER_LENGTH;
	++buffer->top;
	return index;
}

float points_distance(float x1, float y1, float x2, float y2) {
	float dx = x1 - x2;
	float dy = y1 - y2;
	return sqrtf(dx*dx + dy*dy);
}

float wrap_angle_pi(float angle) {
	int k = ceilf(-angle / (2.0 * M_PI) - 0.5);
	return angle + 2.0 * M_PI * k;
}

// Airplane queue
void airplane_queue_init(airplane_queue_t* queue) {
	queue->top = 0;
	queue->bottom = 0;
	pthread_mutex_init(&queue->mutex, NULL);
}

bool airplane_queue_is_empty_unsafe(const airplane_queue_t* queue) {
	return queue->top == queue->bottom;
}

bool airplane_queue_is_full_unsafe(const airplane_queue_t* queue) {
	return ((queue->bottom + 1) % AIRPLANE_QUEUE_LENGTH) == queue->top;
}

int airplane_queue_push(airplane_queue_t* queue, shared_airplane_t* new_value){
	int rv = SUCCESS;

	pthread_mutex_lock(&queue->mutex);
	if (!airplane_queue_is_full_unsafe(queue)) {
		queue->elems[queue->bottom] = new_value;
		queue->bottom = (queue->bottom + 1) % AIRPLANE_QUEUE_LENGTH;
		rv = SUCCESS;
	} else {
		rv = ERROR_GENERIC;
	}
	pthread_mutex_unlock(&queue->mutex);
	return rv;
}

shared_airplane_t* airplane_queue_pop(airplane_queue_t* queue){
	shared_airplane_t* elem = NULL;

	pthread_mutex_lock(&queue->mutex);
	if (!airplane_queue_is_empty_unsafe(queue)) {
		elem = queue->elems[queue->top];
		queue->elems[queue->top] = NULL;
		queue->top = (queue->top + 1) % AIRPLANE_QUEUE_LENGTH;
	}
	pthread_mutex_unlock(&queue->mutex);
	return elem;
}

bool airplane_queue_is_empty(airplane_queue_t* queue) {
	bool is_empty = false;
	pthread_mutex_lock(&queue->mutex);
	is_empty = airplane_queue_is_empty_unsafe(queue);
	pthread_mutex_unlock(&queue->mutex);
	return is_empty;
}

bool airplane_queue_is_full(airplane_queue_t* queue) {
	bool is_full = false;
	pthread_mutex_lock(&queue->mutex);
	is_full = airplane_queue_is_full(queue);
	pthread_mutex_unlock(&queue->mutex);
	return is_full;
}

int airplane_queue_copy(airplane_queue_t* queue,
		airplane_t* dst_array, int array_size) {
	shared_airplane_t* airplanes_ptrs[AIRPLANE_QUEUE_LENGTH] = { 0 };
	int i = 0;
	int j = 0;
	int n = 0;

	pthread_mutex_lock(&queue->mutex);
	i = 0;
	j = queue->top;
	while (i < AIRPLANE_QUEUE_LENGTH && i < array_size && j != queue->bottom) {
		airplanes_ptrs[i] = queue->elems[j];
		++i;
		j = (j + 1) % AIRPLANE_QUEUE_LENGTH;
	}	
	pthread_mutex_unlock(&queue->mutex);
	
	n = i;
	for (i = 0; i < n; ++i) {
		pthread_mutex_lock(&airplanes_ptrs[i]->mutex);
		dst_array[i] = airplanes_ptrs[i]->airplane;
		pthread_mutex_unlock(&airplanes_ptrs[i]->mutex);
	}
	return n;
}

void airplane_queue_print(airplane_queue_t* queue) {
	int i = 0;

	pthread_mutex_lock(&queue->mutex);
	for (i = 0; i < AIRPLANE_QUEUE_LENGTH; ++i) {
		if (queue->elems[i]) {
			printf("%2d ", queue->elems[i]->airplane.unique_id);
		} else {
			printf(" * ");
		}
	}
	printf("\n");
	for (i = 0; i < AIRPLANE_QUEUE_LENGTH; ++i) {
		if (queue->top == i && queue->bottom == i) {
			printf("TB ");
		} else if (queue->top == i) {
			printf(" T ");
		} else if (queue->bottom == i) {
			printf(" B ");
		} else {
			printf("   ");
		}
	}
	printf("\n");
	pthread_mutex_unlock(&queue->mutex);
}
// Airplane pool
void airplane_pool_init(airplane_pool_t* pool) {
	int i = 0;

	for (i = 0; i < AIRPLANE_POOL_SIZE; ++i) {
		pool->elems[i].airplane.unique_id = i;
		pool->is_free[i] = true;
	}

	pthread_mutex_init(&pool->mutex, NULL);
	pool->n_free = AIRPLANE_POOL_SIZE;
}

shared_airplane_t* airplane_pool_get_new(airplane_pool_t* pool) {
	shared_airplane_t* elem = NULL;
	int i = 0;

	pthread_mutex_lock(&pool->mutex);
	while (i < AIRPLANE_POOL_SIZE && pool->is_free[i] == false) {
		++i;
	}

	if (i < AIRPLANE_POOL_SIZE) {
		elem = &pool->elems[i];
		pool->is_free[i] = false;
		--pool->n_free;
	}
	pthread_mutex_unlock(&pool->mutex);
	return elem;
}

void airplane_pool_free(airplane_pool_t* pool, shared_airplane_t* elem) {
	assert(elem >= &(pool->elems[0]) && elem < &(pool->elems[AIRPLANE_POOL_SIZE]));
	int i = elem - &pool->elems[0];

	pthread_mutex_lock(&pool->mutex);
	pool->is_free[i] = true;
	++pool->n_free;
	pthread_mutex_unlock(&pool->mutex);
}

void update_airplane_trail(const airplane_t* airplane, cbuffer_t* trail) {
	point2i_t new_point;
	int i = 0;
	convert_coord_to_display(airplane->x, airplane->y,
		&new_point.x, &new_point.y);
	
	i = cbuffer_next_index(trail);
	trail->points[i] = new_point;
}

void init_trail_buffer(cbuffer_t* trail) {
	int i = 0;
	trail->top = 0;
	for (i = 0; i < TRAIL_BUFFER_LENGTH; ++i) {
		trail->points[i].x = 0;
		trail->points[i].y = 0;
	}
}

void handle_trails(BITMAP* bitmap, airplane_t* airplanes, int n_airplanes,
		cbuffer_t* trails, bool show_trails) {
	int i = 0;
	bool trails_updated[MAX_AIRPLANE] = { 0 };
	airplane_t* airplane = NULL;
	cbuffer_t* trail = NULL;

	for (i = 0; i < n_airplanes; ++i) {
		airplane = &airplanes[i];
		trail = &trails[airplane->unique_id];
		trails_updated[airplane->unique_id] = true;
		update_airplane_trail(airplane, trail);
		if (show_trails)
			draw_trail(bitmap, trail, TRAIL_BUFFER_LENGTH, TRAIL_COLOR);
	}

	// Resetting unused trail buffers
	for (i = 0; i < MAX_AIRPLANE; ++i) {
		if (trails_updated[i] == false)
			init_trail_buffer(&trails[i]);
	}
}

void toggle_trails() {
	show_trails = !show_trails;
}

void toggle_next_waypoint() {
	show_next_waypoint = !show_next_waypoint;
}

void update_task_states(const task_info_t* task_info) {
	task_states[task_info->task_num].deadline_miss = task_info->deadline_miss;
}