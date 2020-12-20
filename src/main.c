#include <stdbool.h>
#include <allegro.h>
#include <pthread.h>
#include <math.h>
#include <stdio.h>

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
struct task_info airplane_task_infos[MAX_AIRPLANE];
shared_airplane_t airplanes[MAX_AIRPLANE];
int n_airplanes = 0;		// Number of airplane in the airplanes array
bool end = false;			// true if the program should terminate

// ==================================================================
//                      FUNCTIONS DECLARATION
// ==================================================================
void init();
void init_holding_trajectory();
void create_tasks(task_info_t* graphic_task_info, task_info_t* input_task_info);
void join_tasks(task_info_t* graphic_task_info, task_info_t* input_task_info);
void spawn_inbound_airplane();
const waypoint_t* trajectory_get_point(const trajectory_t* trajectory, int index);
int cbuffer_next_index(cbuffer_t* buffer);

// Airplane control
void compute_airplane_controls(const airplane_t* airplane,
	const waypoint_t* des_point, float* vel_cmd, float* omega_cmd);
void update_airplane_state(airplane_t* airplane, float vel_cmd, 
	float omega_cmd);
void update_airplane_traj_index(airplane_t* airplane, 
	const waypoint_t* des_point);
float wrap_angle_pi(float angle);
float points_distance(float x1, float y1, float x2, float y2);

// Task functions
void* graphic_task(void* arg);
void* airplane_task(void* arg);
void* input_task(void* arg);


// ==================================================================
//                    			MAIN
// ==================================================================
int main() {
	struct task_info graphic_task_info;
	struct task_info input_task_info;

	init();
	create_tasks(&graphic_task_info, &input_task_info);
	join_tasks(&graphic_task_info, &input_task_info);

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
	BITMAP* status_box = create_status_box();

	int local_n_airplanes = 0;
	airplane_t local_airplanes[MAX_AIRPLANE];
	cbuffer_t airplane_trails[MAX_AIRPLANE];

	task_set_activation(task_info);

	while (!end) {
		for (i = 0; i < local_n_airplanes; ++i) {
			draw_airplane(&local_airplanes[i], BG_COLOR);
		}
		local_n_airplanes = n_airplanes;

		for (i = 0; i < n_airplanes; ++i) {			
			pthread_mutex_lock(&airplanes[i].mutex);
			local_airplanes[i] = airplanes[i].airplane;
			pthread_mutex_unlock(&airplanes[i].mutex);
			
			handle_airplane_trail(&local_airplanes[i], &airplane_trails[i]);
			draw_airplane(&local_airplanes[i], AIRPLANE_COLOR);
		}

		update_status_box(status_box, counter);
		blit_status_box(status_box);
		
		++counter;

		if (task_deadline_missed(task_info)) {
			fprintf(stderr, "Graphic task - Deadline miss\n");
		}
		task_wait_for_activation(task_info);
	}

	printf("Exiting...\n");
	destroy_bitmap(status_box);
  return NULL;
}

// ==================================================================
//                           AIRPLANE TASK
// ==================================================================
void* airplane_task(void* arg) {
	struct task_info* task_info = (struct task_info*) arg;
	shared_airplane_t* global_airplane_ptr = (shared_airplane_t*) task_info->arg;
	float vel_cmd = 0;
	float omega_cmd = 0;
	const waypoint_t* des_point;
	airplane_t local_airplane = global_airplane_ptr->airplane;

	task_set_activation(task_info);

	while (!end) {
		// Updating the local copy of the airplane struct
		pthread_mutex_lock(&global_airplane_ptr->mutex);
		local_airplane = global_airplane_ptr->airplane;
		pthread_mutex_unlock(&global_airplane_ptr->mutex);

		// Computing control; updating the state; updating the reference point
		des_point = trajectory_get_point(
			local_airplane.des_traj, local_airplane.traj_index);
		compute_airplane_controls(&local_airplane, des_point, &vel_cmd, &omega_cmd);
		update_airplane_state(&local_airplane, vel_cmd, omega_cmd);
		update_airplane_traj_index(&local_airplane, des_point);

		// Updating the global airplane struct
		pthread_mutex_lock(&global_airplane_ptr->mutex);
		global_airplane_ptr->airplane = local_airplane;
		pthread_mutex_unlock(&global_airplane_ptr->mutex);

		// Ending task instance
		if (task_deadline_missed(task_info)) {
			fprintf(stderr, "Airplane task deadline missed\n");
		}
		task_wait_for_activation(task_info);
	}

	return NULL;
}

// ==================================================================
//                           INPUT TASK
// ==================================================================
void* input_task(void* arg) {
	char scan = '\0';
	char ascii = '\0';
	bool got_key = false;

  	do {
  	  got_key =  get_keycodes(&scan, &ascii);
  	  if (got_key && scan == KEY_O) {
  	    printf("Key O pressed\n");
  	  } else if (got_key && scan == KEY_I) {
  	    printf("Key I pressed\n");
		spawn_inbound_airplane();
  	  }
  	} while (scan != KEY_ESC);

	printf("Exiting...\n");
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
		point->angle = s + M_PI;
		s += HOLDING_TRAJECTORY_STEP;
	}
}

void create_tasks(task_info_t* graphic_task_info, task_info_t* input_task_info) {
	int err = 0;

	// Creating graphic task
	task_info_init(graphic_task_info, 1, 
		GRAPHIC_PERIOD_MS, GRAPHIC_PERIOD_MS, PRIORITY);
	err = task_create(graphic_task_info, graphic_task);
	if (err) fprintf(stderr, ERR_MSG_TASK_CREATE, "graphic task", err);

	// Creating input task
	task_info_init(input_task_info, MAX_AIRPLANE, 
		INPUT_PERIOD_MS, INPUT_PERIOD_MS, PRIORITY);
	err = task_create(input_task_info, input_task);
	if (err) fprintf(stderr, ERR_MSG_TASK_CREATE, "input task", err);
}

void join_tasks(task_info_t* graphic_task_info, task_info_t* input_task_info) {
	int err = 0;
	int i = 0;

	// Joining graphic task
	err = task_join(graphic_task_info, NULL);
	if (err) fprintf(stderr, ERR_MSG_TASK_JOIN, "graphic task", err);

	// Joining input task
	err = task_join(input_task_info, NULL);
	if (err) fprintf(stderr, ERR_MSG_TASK_JOIN, "input task", err);

	// Joining airplane tasks
	for (i = 0; i < n_airplanes; ++i) {
		err = task_join(&airplane_task_infos[i], NULL);
		if (err) fprintf(stderr, ERR_MSG_TASK_JOIN_AIR, i, err);
	}
}

void spawn_inbound_airplane() {
	if (n_airplanes >= MAX_AIRPLANE) return;
	int i = n_airplanes;
	int err = 0;

	airplanes[i].airplane = (airplane_t) {
		.x = 0,
		.y = 0,
		.angle = 0,
		.des_traj = &holding_trajectory,
		.traj_index = 0
	};
	pthread_mutex_init(&airplanes[i].mutex, NULL);

	task_info_init(&airplane_task_infos[i], 2 + i, 
		AIRPLANE_PERIOD_MS, AIRPLANE_PERIOD_MS, PRIORITY - 1);
	airplane_task_infos[i].arg = &airplanes[i];

	err = task_create(&airplane_task_infos[i], airplane_task);
	if (err) fprintf(stderr, "Errore while creating the task. Errno %d\n", err);
	++n_airplanes;
}

void compute_airplane_controls(const airplane_t* airplane, 
		const waypoint_t* des_point, float* vel_cmd, float* omega_cmd) {
	double des_angle = 0.0;
	double angle_error = 0.0;
	
	if (des_point) {
		des_angle = atan2f(des_point->y - airplane->y,
							des_point->x - airplane->x);
		angle_error = wrap_angle_pi(des_angle - airplane->angle);
		*omega_cmd = AIRPLANE_CTRL_OMEGA_GAIN * angle_error;
	} else {
		*omega_cmd = 0;
	}
	*vel_cmd = AIRPLANE_CTRL_VEL;
}

void update_airplane_state(airplane_t* airplane, float vel_cmd, 
		float omega_cmd) {
	airplane->x += vel_cmd * cosf(airplane->angle) * AIRPLANE_CTRL_SIM_PERIOD;
	airplane->y += vel_cmd * sinf(airplane->angle) * AIRPLANE_CTRL_SIM_PERIOD;
	airplane->angle += wrap_angle_pi(omega_cmd * AIRPLANE_CTRL_SIM_PERIOD);
}

void update_airplane_traj_index(airplane_t* airplane,
		const waypoint_t* des_point) {
	float distance = 0.0;

	if (des_point) {
		distance = points_distance(airplane->x, airplane->y,
			des_point->x, des_point->y);
		
		if (distance < AIRPLANE_CTRL_MIN_DIST) ++airplane->traj_index;
	}
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