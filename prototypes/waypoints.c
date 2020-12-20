#include <stdbool.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <allegro.h>

#define MAX_WAYPOINTS 		50
#define TRAJECTORY_POINTS 	50
#define TRAJECTORY_STEP 	2.0 * M_PI / TRAJECTORY_POINTS
#define TRAJECTORY_RADIUS 	5
#define SCREEN_WIDTH		1024
#define SCREEN_HEIGHT		720
#define BG_COLOR			0     // black

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

trajectory_t simple_trajectory;

const waypoint_t* trajectory_get(const trajectory_t* trajectory, int index) {
	if (trajectory == NULL) return NULL;
	if (index >= 0 && index < trajectory->size) {
		printf("%2d - ", index);
		return &trajectory->waypoints[index];
	}
	if (trajectory->is_cyclic) {
		index = index % trajectory->size;
		return &trajectory->waypoints[index];
	}

	return NULL;
}

void init_trajectory() {
	int i = 0;
	float s = 0;
	waypoint_t* point;

	simple_trajectory.size = TRAJECTORY_POINTS;
	simple_trajectory.is_cyclic = true;
	for (i = 0; i < TRAJECTORY_POINTS; ++i) {
		point = &simple_trajectory.waypoints[i];
		point->x = TRAJECTORY_RADIUS * cosf(s);
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
}

void draw_point(const waypoint_t* point, int color) {
	int x = 50.0 * (point->x) + SCREEN_WIDTH/2.0;
	int y = 50.0 * (-point->y) + SCREEN_HEIGHT/2.0;
	circlefill(screen, x, y, 2, color);
}

int main() {
	const waypoint_t* point;
	
	init();
	init_trajectory();
	for (int i = 0; i < 50; ++i) {
		point = trajectory_get(&simple_trajectory, i);
		printf("%2d) %5.2f %5.2f %5.2f\n", i, point->x, point->y, point->angle);
		draw_point(point, 11);
	}

	readkey();
	return 0;
}