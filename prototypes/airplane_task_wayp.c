#include <allegro.h>
#include <stdbool.h>
#include <stdio.h>
#include <math.h>
#include <pthread.h>

#include "ptask.h"
#include "graphics.h"

// GRAPHIC CONSTANTS
#define SCREEN_WIDTH			1024
#define SCREEN_HEIGHT			720
#define BG_COLOR				0     // black
#define MAIN_COLOR				14		// yellow
#define STATUS_BOX_WIDTH		SCREEN_WIDTH - SCREEN_HEIGHT - 20
#define STATUS_BOX_HEIGHT		SCREEN_HEIGHT - 20
#define STATUS_BOX_X			SCREEN_HEIGHT + 10
#define STATUS_BOX_Y			10
#define STATUS_BOX_PADDING 		5
#define AIRPLANE_SIZE			10
#define MAX_AIRPLANE			10
#define TRAIL_BUFFER_LENGTH		100

#define MAX_WAYPOINTS 			50
#define TRAJECTORY_POINTS 		30
#define TRAJECTORY_STEP 		2.0 * M_PI_F/ TRAJECTORY_POINTS
#define TRAJECTORY_RADIUS 		70

#define AIRPLANE_CTRL_OMEGA_GAIN	0.8
#define AIRPLANE_CTRL_MIN_DIST		20.0
#define AIRPLANE_CTRL_VEL			15.0

#define GRAPHIC_PERIOD_MS		30
#define PERIOD_MS     			20
#define PRIORITY      			50

typedef struct {
	float x;
	float y;
	float angle;
} waypoint_t;

typedef struct {
	waypoint_t waypoints[MAX_WAYPOINTS];
	bool is_cyclic;
	int size;
} trajectory_t;

typedef struct {
	float x;
	float y;
	float angle;
	const trajectory_t* des_traj;
	int traj_index;
} airplane_t;

typedef struct {
	airplane_t airplane;
	pthread_mutex_t mutex;
} shared_airplane_t;

// 2D Point with Interger coordinates
typedef struct {
	int x;
	int y;
} point2i_t;

typedef struct  {
	point2i_t points[TRAIL_BUFFER_LENGTH];
	int top;
} cbuffer_t;

const waypoint_t* trajectory_get(const trajectory_t* trajectory, int index) {
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

// GLOBAL VARIABLES
trajectory_t trajectory;
shared_airplane_t airplanes[MAX_AIRPLANE];
int n_airplanes = 0;
bool end = false;
struct task_info airplane_task_infos[MAX_AIRPLANE];

void init_trajectory() {
	int i = 0;
	float s = 0.0;
	waypoint_t* point;

	trajectory.size = TRAJECTORY_POINTS;
	trajectory.is_cyclic = true;
	for (i = 0; i < TRAJECTORY_POINTS; ++i) {
		point = &trajectory.waypoints[i];
		point->x = TRAJECTORY_RADIUS * cosf(s);
		if (s < M_PI_2_F ||s >= 3.0 * M_PI_2_F) point->x += 100;
		else point->x -= 40;
		point->y = TRAJECTORY_RADIUS * sinf(s);
		point->angle = s + M_PI;
		s += TRAJECTORY_STEP;
	}
}

void init() {
	allegro_init();
	install_keyboard();

	set_color_depth(8);
	set_gfx_mode(GFX_AUTODETECT_WINDOWED, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 0);
	clear_to_color(screen, BG_COLOR);

	init_trajectory();
	/*
	pthread_mutex_init(&airplanes[0].mutex, NULL);
	pthread_mutex_init(&airplanes[1].mutex, NULL);
	pthread_mutex_init(&airplanes[2].mutex, NULL);
	airplanes[0].airplane = (airplane_t) {
		.x = 0,
		.y = 0,
		.angle = -M_PI_4,
		.des_traj = &trajectory,
		.traj_index = 5
	};
	airplanes[1].airplane = (airplane_t) {
		.x = 10,
		.y = -20,
		.angle = -M_PI_4,
		.des_traj = &trajectory,
		.traj_index = 0
	};
	airplanes[2].airplane = (airplane_t) {
		.x = 10,
		.y = 20,
		.angle = -M_PI_4,
		.des_traj = &trajectory,
		.traj_index = 10
	};
	*/
}

BITMAP* create_status_box() {
	BITMAP* status_box = create_bitmap(STATUS_BOX_WIDTH, STATUS_BOX_HEIGHT);
	clear_to_color(status_box, BG_COLOR);
	rect(status_box, 0, 0, STATUS_BOX_WIDTH - 1, STATUS_BOX_HEIGHT - 1, MAIN_COLOR);
	return status_box;
}

void blit_status_box(BITMAP* status_box) {
	blit(status_box, screen, 0, 0, STATUS_BOX_X, STATUS_BOX_Y,
			STATUS_BOX_WIDTH, STATUS_BOX_HEIGHT);
}

void rotate_point(float* x, float* y, float xc, float yc, float cos_angle, float sin_angle) {
	float tmp_x;
	*x -= xc;
	*y -= yc;
	tmp_x = *x;

	*x = cos_angle * tmp_x + sin_angle * (*y);
	*y = -sin_angle * tmp_x + cos_angle * (*y);
	
	*x += xc;
	*y += yc;
}

void draw_triangle(BITMAP* bitmap, int xc, int yc, int radius, float angle,
		int color) {
	float sqrt_3 = sqrt(3.0);
	float sin_angle = sinf(angle);
	float cos_angle = cosf(angle);
	float x1 = sqrt_3 * radius / 2.0 + xc;
	float y1 = ((float) radius) / 2.0 + yc;
	float x2 = xc;
	float y2 = yc - radius;
	float x3 =  -sqrt_3 * radius / 2.0 + xc;
	float y3 = y1;
	rotate_point(&x1, &y1, xc, yc, cos_angle, sin_angle);
	rotate_point(&x2, &y2, xc, yc, cos_angle, sin_angle);
	rotate_point(&x3, &y3, xc, yc, cos_angle, sin_angle);
	triangle(bitmap,
			roundf(x1), roundf(y1),
			roundf(x2), roundf(y2),
			roundf(x3), roundf(y3), color);

}

void* airplane_task(void* arg) {
	struct task_info* task_info = (struct task_info*) arg;
	shared_airplane_t* global_airplane_ptr = (shared_airplane_t*) task_info->arg;
	float v_control = 0;
	float omega_control = 0;
	float angle_error = 0;
	float dt = 0.020;
	float des_angle = 0.0;
	float distance = 0.0;
	const waypoint_t* des_point;
	airplane_t local_airplane = global_airplane_ptr->airplane;

	task_set_activation(task_info);

	while (!end) {
		pthread_mutex_lock(&global_airplane_ptr->mutex);
		local_airplane = global_airplane_ptr->airplane;
		pthread_mutex_unlock(&global_airplane_ptr->mutex);

		des_point = trajectory_get(&trajectory, local_airplane.traj_index);
		if (des_point) {
			des_angle = atan2f(des_point->y - local_airplane.y,
											 	des_point->x - local_airplane.x);
			angle_error = wrap_angle_pi(des_angle - local_airplane.angle);
			omega_control = AIRPLANE_CTRL_OMEGA_GAIN * angle_error;
		} else {
			omega_control = 0;
		}
		v_control = AIRPLANE_CTRL_VEL;		

		local_airplane.x += v_control * cosf(local_airplane.angle) * dt;
		local_airplane.y += v_control * sinf(local_airplane.angle) * dt;
		local_airplane.angle += wrap_angle_pi(omega_control * dt);

		if (des_point) {
			distance = points_distance(local_airplane.x, local_airplane.y, des_point->x, des_point->y);
			if (distance < AIRPLANE_CTRL_MIN_DIST) {
				++local_airplane.traj_index;
			}
		}

		pthread_mutex_lock(&global_airplane_ptr->mutex);
		global_airplane_ptr->airplane = local_airplane;
		pthread_mutex_unlock(&global_airplane_ptr->mutex);

		if (task_deadline_missed(task_info)) {
			fprintf(stderr, "Airplane task deadline missed\n");
		}

		task_wait_for_activation(task_info);
	}

	return NULL;
}

void convert_coord_to_display(int src_x, int src_y, int* dst_x, int* dst_y) {
	*dst_x = src_x + SCREEN_HEIGHT/2.0;
	*dst_y = -src_y + SCREEN_HEIGHT/2.0;
}

void draw_airplane(const airplane_t* airplane, int color) {
	int x = 0;
	int y = 0;
	float angle = (airplane->angle - M_PI_2_F);
	convert_coord_to_display(airplane->x, airplane->y, &x, &y);
	draw_triangle(screen, x, y, AIRPLANE_SIZE, angle, color);
}

void draw_point(const waypoint_t* point, int color) {
	int x = 0;
	int y = 0;
	convert_coord_to_display(point->x, point->y, &x, &y);
	circlefill(screen, x, y, 2, color);
}

int cbuffer_next_index(cbuffer_t* buffer) {
	int index = (buffer->top + 1) % TRAIL_BUFFER_LENGTH;
	++buffer->top;
	return index;
}

void draw_trail(const cbuffer_t* trails, int n, int color) {
	int i = 0;
	int idx = 0;
	
	if (n > TRAIL_BUFFER_LENGTH) n = TRAIL_BUFFER_LENGTH;
	
	for (i = 0; i < n; ++i) {
		idx = (trails->top + TRAIL_BUFFER_LENGTH - i) % TRAIL_BUFFER_LENGTH;
		putpixel(screen, trails->points[idx].x, trails->points[idx].y, color);
	}
}

void handle_airplane_trail(const airplane_t* airplane,  cbuffer_t* trail) {
	point2i_t new_point;
	int i = 0;
	convert_coord_to_display(airplane->x, airplane->y,
		&new_point.x, &new_point.y);
	
	i = cbuffer_next_index(trail);
	putpixel(screen, trail->points[i].x, trail->points[i].y, BG_COLOR);
	trail->points[i] = new_point;
	draw_trail(trail, TRAIL_BUFFER_LENGTH, 5);
}

void* graphic_task(void* arg) {
	struct task_info* task_info = (struct task_info*) arg;
	char str[100] = { 0 };
	int counter = 0;
	BITMAP* status_box = create_status_box();

	airplane_t local_airplanes[MAX_AIRPLANE];
	int local_n_airplanes = 0;
	cbuffer_t airplane_trails[MAX_AIRPLANE];
	// const waypoint_t* des_point;

	for (int i = 0; i < local_n_airplanes; ++i) {
		pthread_mutex_lock(&airplanes[i].mutex);
		local_airplanes[i] = airplanes[i].airplane;
		pthread_mutex_unlock(&airplanes[i].mutex);
		airplane_trails[i].top = 0;
	}

	task_set_activation(task_info);

	while (!end) {
		for (int i = 0; i < n_airplanes; ++i) {
			if (i < local_n_airplanes)
				draw_airplane(&local_airplanes[i], BG_COLOR);
			
			pthread_mutex_lock(&airplanes[i].mutex);
			local_airplanes[i] = airplanes[i].airplane;
			pthread_mutex_unlock(&airplanes[i].mutex);
			
			handle_airplane_trail(&local_airplanes[i], &airplane_trails[i]);
			draw_airplane(&local_airplanes[i], 11);
			// des_point = trajectory_get(&trajectory, local_airplanes[i].traj_index);
			// if (des_point) draw_point(des_point, 4);
			
		}

		textout_ex(status_box, font, str,
				STATUS_BOX_PADDING, STATUS_BOX_PADDING, BG_COLOR, BG_COLOR);
		sprintf(str, "Messaggio di prova %d", counter);
		textout_ex(status_box, font, str,
				STATUS_BOX_PADDING, STATUS_BOX_PADDING, MAIN_COLOR, BG_COLOR);
		blit_status_box(status_box);
		
		++counter;
		local_n_airplanes = n_airplanes;

		if (task_deadline_missed(task_info)) {
			fprintf(stderr, "Graphic task - Deadline miss\n");
		}
		task_wait_for_activation(task_info);
	}

	printf("Exiting...\n");
	destroy_bitmap(status_box);
  return NULL;
}

void spawn_inbound_airplane() {
	if (n_airplanes >= MAX_AIRPLANE) return;
	int i = n_airplanes;
	int err = 0;

	airplanes[i].airplane = (airplane_t) {
		.x = 0,
		.y = 0,
		.angle = 0,
		.des_traj = &trajectory,
		.traj_index = 0
	};
	pthread_mutex_init(&airplanes[i].mutex, NULL);

	task_info_init(&airplane_task_infos[i], 2 + i, PERIOD_MS, PERIOD_MS, PRIORITY - 1);
	airplane_task_infos[i].arg = &airplanes[i];
	err = task_create(&airplane_task_infos[i], airplane_task);
	if (err) fprintf(stderr, "Errore while creating the task. Errno %d\n", err);
	++n_airplanes;
}

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

int main() {
	int err = 0;
	struct task_info graphic_task_info;
	struct task_info input_task_info;

	init();

	task_info_init(&graphic_task_info, 1, GRAPHIC_PERIOD_MS, GRAPHIC_PERIOD_MS, PRIORITY);
	err = task_create(&graphic_task_info, graphic_task);
	if (err) fprintf(stderr, "Errore while creating the task. Errno %d\n", err);

	/*
	for (int i = 0; i < n_airplanes; ++i) {
		task_info_init(&airplane_task_infos[i], 2 + i, PERIOD_MS, PERIOD_MS, PRIORITY - 1);
		airplane_task_info[i].arg = &airplanes[i];
		err = task_create(&airplane_task_info[i], airplane_task);
		if (err) fprintf(stderr, "Errore while creating the task. Errno %d\n", err);
	}*/

	task_info_init(&input_task_info, n_airplanes, PERIOD_MS, PERIOD_MS, PRIORITY);
	err = task_create(&input_task_info, input_task);
	if (err) fprintf(stderr, "Errore while creating the task. Errno %d\n", err);

	// JOINING
	err = task_join(&graphic_task_info, NULL);
	if (err) fprintf(stderr, "Errore while joining the graphic task. Errno %d\n", err);

	for (int i = 0; i < n_airplanes; ++i) {
		err = task_join(&airplane_task_infos[i], NULL);
		if (err) fprintf(stderr, "Errore while joining the airplane task. Errno %d\n", err);
	}

	err = task_join(&input_task_info, NULL);
	if (err) fprintf(stderr, "Errore while joining the task. Errno %d\n", err);

	allegro_exit();
	return 0;
}