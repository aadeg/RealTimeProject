/*
 * graphics.c
 * 
 * Definition of graphic-related functions declared in graphics.h
 */

#include <stdio.h>
#include <math.h>

#include "graphics.h"
#include "consts.h"

#define SQRT_3		1.732050808f

static BITMAP* main_bg_bitmap = NULL;	// background image of the main box

// Sidebar sections limits
static int sidebar_box_system_state_y_start = 0;
static int sidebar_box_system_state_y_end = 0;
static int sidebar_box_tasks_state_y_start = 0;
static int sidebar_box_tasks_state_y_end = 0;


// ==================================================================
//                            MAIN BOX
// =================================================================
// Create and return a BITMAP containing the main scene
BITMAP* create_main_box(void) {
	BITMAP* main_box = create_bitmap(MAIN_BOX_WIDTH, MAIN_BOX_HEIGHT);
	clear_main_box(main_box);
	rect(main_box, 0, 0, MAIN_BOX_WIDTH - 1, MAIN_BOX_HEIGHT - 1,
		MAIN_COLOR);
	return main_box;
}

// Clear the main box bitmap with the background image
void clear_main_box(BITMAP* main_box) {
	if (main_bg_bitmap == NULL)
		main_bg_bitmap = load_bitmap("assets/background.bmp", NULL);
	blit(main_bg_bitmap, main_box, 0, 0, 0, 0, MAIN_BOX_WIDTH, MAIN_BOX_HEIGHT);
}

// Draw the main box to the screen
void blit_main_box(BITMAP* main_box) {
	rect(main_box, 0, 0, MAIN_BOX_WIDTH - 1, MAIN_BOX_HEIGHT - 1,
		MAIN_COLOR);
	blit(main_box, screen, 0, 0, MAIN_BOX_X, MAIN_BOX_Y,
		MAIN_BOX_WIDTH, MAIN_BOX_HEIGHT);
}

// ==================================================================
//                         SIDEBAR BOX
// ==================================================================
// Internal utility function
void _sidebar_textout_ex(BITMAP* sidebar_box, const char* str, int y) {
	textout_ex(sidebar_box, font, str, SIDEBAR_BOX_PADDING, y,
		MAIN_COLOR, BG_COLOR);
}

// Create and return a BITMAP used as sidebar
BITMAP* create_sidebar_box(void) {
	BITMAP* sidebar_box = create_bitmap(SIDEBAR_BOX_WIDTH, SIDEBAR_BOX_HEIGHT);
	int y = SIDEBAR_BOX_PADDING;

	clear_to_color(sidebar_box, BG_COLOR);
	rect(sidebar_box, 0, 0, SIDEBAR_BOX_WIDTH - 1, SIDEBAR_BOX_HEIGHT - 1,
		MAIN_COLOR);

	// Keyboard commands
	_sidebar_textout_ex(sidebar_box, "KEYBOARD COMMANDS", y);
	y += SIDEBAR_BOX_VSPACE + SIDEBAR_BOX_PADDING;
	_sidebar_textout_ex(sidebar_box, "I: spawn an Inbound airplane", y);
	y += SIDEBAR_BOX_VSPACE;
	_sidebar_textout_ex(sidebar_box, "O: spawn an Outbound airplane", y);
	y += SIDEBAR_BOX_VSPACE;
	_sidebar_textout_ex(sidebar_box, "T: show / hide trails", y);
	y += SIDEBAR_BOX_VSPACE;
	_sidebar_textout_ex(sidebar_box, "W: show / hide next waypoint", y);
	y += SIDEBAR_BOX_VSPACE + SIDEBAR_BOX_PADDING;
	line(sidebar_box, 0, y, SIDEBAR_BOX_WIDTH, y, MAIN_COLOR);
	y += SIDEBAR_BOX_PADDING;

	// System state
	_sidebar_textout_ex(sidebar_box, "SYSTEM STATE", y);
	y += SIDEBAR_BOX_VSPACE + SIDEBAR_BOX_PADDING;
	sidebar_box_system_state_y_start = y;
	y += 3 * SIDEBAR_BOX_VSPACE;
	sidebar_box_system_state_y_end = y;
	y += SIDEBAR_BOX_PADDING;
	line(sidebar_box, 0, y, SIDEBAR_BOX_WIDTH, y, MAIN_COLOR);
	y += SIDEBAR_BOX_PADDING;

	// Task state
	_sidebar_textout_ex(sidebar_box, "TASKS STATE", y);
	y += SIDEBAR_BOX_VSPACE + SIDEBAR_BOX_PADDING;
	sidebar_box_tasks_state_y_start = y;
	sidebar_box_tasks_state_y_end = SIDEBAR_BOX_HEIGHT - SIDEBAR_BOX_PADDING;

	return sidebar_box;
}

// Draw the sidebar box to the screen
void blit_sidebar_box(BITMAP* sidebar_box) {
	blit(sidebar_box, screen, 0, 0, SIDEBAR_BOX_X, SIDEBAR_BOX_Y,
		SIDEBAR_BOX_WIDTH, SIDEBAR_BOX_HEIGHT);
}

// Update the sidebar box with the new information
void update_sidebar_box(BITMAP* sidebar_box, shared_system_state_t* system_state,
		task_state_t* task_states, const int task_states_size) {
	update_sidebar_system_state(sidebar_box, system_state);
	update_sidebar_tasks_state(sidebar_box, task_states, task_states_size);
}

// Update the system state in the sidebar box
void update_sidebar_system_state(BITMAP* sidebar_box,
		shared_system_state_t* system_state) {
	char str[SIDEBAR_STR_LENGTH] = { 0 };
	int i = 0;
	int y = sidebar_box_system_state_y_start;	// text y-coordinate
	system_state_t local_system_state;

	pthread_mutex_lock(&system_state->mutex);
	local_system_state = system_state->state;
	pthread_mutex_unlock(&system_state->mutex);

	// Clearing the old information
	rect(sidebar_box,
		SIDEBAR_BOX_PADDING, sidebar_box_system_state_y_start, 
		SIDEBAR_BOX_WIDTH - SIDEBAR_BOX_PADDING, sidebar_box_system_state_y_end,
		BG_COLOR);

	// Writing the number of airplanes in the system
	sprintf(str, "Airplanes:  %02d / %02d", local_system_state.n_airplanes, MAX_AIRPLANE);
	_sidebar_textout_ex(sidebar_box, str, y);
	y += SIDEBAR_BOX_VSPACE;

	// Writing the state of the runways
	for (i = 0; i < N_RUNWAYS; ++i) {
		if (local_system_state.is_runway_free[i])
			sprintf(str, "Runway %d:   %s", i + 1, "free");
		else
			sprintf(str, "Runway %d:   %s", i + 1, "busy");
		_sidebar_textout_ex(sidebar_box, str, y);
		y += SIDEBAR_BOX_VSPACE;
	}
}

// Update the task states in the sidebar box
void update_sidebar_tasks_state(BITMAP* sidebar_box,
		task_state_t* task_states, const int task_states_size) {
	char str[SIDEBAR_STR_LENGTH] = { 0 };
	int i = 0;
	int y = sidebar_box_tasks_state_y_start;	// text y-coordinate
	char state;									// state of the task

	// Clearing the old information
	rect(sidebar_box,
		SIDEBAR_BOX_PADDING, sidebar_box_tasks_state_y_end,
		SIDEBAR_BOX_WIDTH - SIDEBAR_BOX_PADDING, sidebar_box_tasks_state_y_end,
		BG_COLOR);

	// Writing columns header
	sprintf(str, "%-16s %3s %3s", "", "sts", "dlm");
	_sidebar_textout_ex(sidebar_box, str, y);
	y += SIDEBAR_BOX_VSPACE + SIDEBAR_BOX_PADDING;

	// Writing the state of each task
	for (i = 0; i < task_states_size; ++i) {
		state = task_states[i].is_running ? 'R' : 'S';
		sprintf(str, "%-16s  %c  %3d",
			task_states[i].str, state, task_states[i].deadline_miss);
		_sidebar_textout_ex(sidebar_box, str, y);
		y += SIDEBAR_BOX_VSPACE;
	}
}

// ==================================================================
//                        AIRPLANE GRAPHIC
// ==================================================================
// Compute the coordinates of the 3 points of a equilateral triangle centered 
// in (xc, yc), inscribed in a circonference of radius "radius" and rotate 
// by "angle" radiants. xs and ys are the 3-sized arrays that will the contain
// the output of the function
void get_triangle_coord(float xc, float yc, float radius, float angle, int* xs, int* ys) {	
	// Computing the coordinates of the vertices 
	float x1 = SQRT_3 * radius / 2.0f + xc;
	float y1 = (radius) / 2.0f + yc;
	float x2 = xc;
	float y2 = yc - radius;
	float x3 =  -SQRT_3 * radius / 2.0f + xc;
	float y3 = y1;

	// Rotating the points
	float sin_angle = sinf(angle);
	float cos_angle = cosf(angle);
	rotate_point(&x1, &y1, xc, yc, cos_angle, sin_angle);
	rotate_point(&x2, &y2, xc, yc, cos_angle, sin_angle);
	rotate_point(&x3, &y3, xc, yc, cos_angle, sin_angle);

	// Writing the output
	xs[0] = (int) roundf(x1);
	xs[1] = (int) roundf(x2);
	xs[2] = (int) roundf(x3);
	ys[0] = (int) roundf(y1);
	ys[1] = (int) roundf(y2);
	ys[2] = (int) roundf(y3);
}

// Draw an equilateral triangle cented in (xc, yc), inscribed in a circonference
// of radius "radius" and titled rotated by "angle" radiants.
void draw_triangle(BITMAP* bitmap, int xc, int yc, int radius, float angle,
		int color) {
	// Arrays containing the coordinates of the vertices
	int xs[3];
	int ys[3];

	get_triangle_coord((float) xc, (float) yc, (float) radius, (float) angle,
		xs, ys);
	triangle(bitmap, xs[0], ys[0], xs[1], ys[1], xs[2], ys[2], color);
}

// Rotate a point (x, y) around (xc, yc). The rotation angle is defined by
// "cos_angle" and "sin_angle"
void rotate_point(float* x, float* y, float xc, float yc, 
		float cos_angle, float sin_angle) {
	float tmp_x;	// Temp. variable to store x value

	// Shifting (x, y) by (xc, yc)
	*x -= xc;
	*y -= yc;
	tmp_x = *x;

	// Rotation
	*x = cos_angle * tmp_x + sin_angle * (*y);
	*y = -sin_angle * tmp_x + cos_angle * (*y);
	
	// Shifting back (x, y)
	*x += xc;
	*y += yc;
}

// Convert the cartisian coordinates in screen coordinates
void convert_coord_to_display(float src_x, float src_y, int* dst_x, int* dst_y) {
	*dst_x = ((int) roundf(src_x)) + SCREEN_HEIGHT / 2;
	*dst_y = ((int) roundf(-src_y)) + SCREEN_HEIGHT / 2;
}

// Draw an airplane to "bitmap"
void draw_airplane(BITMAP* bitmap, const airplane_t* airplane) {
	int x = 0;			// screen x-coordinate
	int y = 0;			// screen y-coordinate
	float angle = (airplane->angle - M_PI_2_F);
	int airplane_color = get_airplane_color(airplane);

	convert_coord_to_display(airplane->x, airplane->y, &x, &y);
	draw_triangle(bitmap, x, y, AIRPLANE_SIZE, angle, airplane_color);
}

// Draw a waypoint to "bitmap"
void draw_waypoint(BITMAP* bitmap, const waypoint_t* point) {
	int x = 0;			// screen x-coordinate
	int y = 0;			// screen y-coordinate
	convert_coord_to_display(point->x, point->y, &x, &y);
	circlefill(bitmap, x, y, WAYPOINT_RADIUS, WAYPOINT_COLOR);
}

// Draw the trail of an airplane to "bitmap"
void draw_trail(BITMAP* bitmap, const cbuffer_t* trails, int color) {
	int i = 0;
	
	for (i = 0; i < TRAIL_BUFFER_LENGTH; ++i) {
		putpixel(bitmap, trails->points[i].x, trails->points[i].y, color);
	}
}

// Return the airplane color based on the airplane status
int get_airplane_color(const airplane_t* airplane) {
	enum airplane_status status = airplane->status;
	if (status == INBOUND_LANDING || status == OUTBOUND_TAKEOFF)
		return AIRPLANE_RUNWAY_COLOR;
	else
		return AIRPLANE_COLOR;
}

// ==================================================================
//                          KEYBOARD
// ==================================================================
// Put the scan number and the ascii code of the key pressed in "scan" and
// "ascii". "scan" and "ascii" are valid only when true is returned.
bool get_keycodes(char* scan, char* ascii) {
	int key;
	
	if (!keypressed())
		return false;
	
	key = readkey();		// block until a key is pressed
	*ascii = (char) (key & 0xFF);
	*scan = (char) (key >> 8);
	return true;
}