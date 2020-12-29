#ifndef _GRAPHICS_H_
#define _GRAPHICS_H_

#include <stdbool.h>
#include <allegro.h>

#include "structs.h"
#include "ptask.h"

// ==================================================================
//                           MAIN BOX
// ==================================================================
BITMAP* create_main_box();
void clear_main_box(BITMAP* main_box);
void blit_main_box(BITMAP* main_box);

// ==================================================================
//                         SIDEBAR BOX
// ==================================================================
BITMAP* create_sidebar_box();
void blit_sidebar_box(BITMAP* sidebar_box);
void update_sidebar_box(BITMAP* sidebar_box,
	shared_system_state_t* system_state,
	task_state_t* task_states, const int task_stats_size);
void update_sidebar_system_state(BITMAP* sidebar_box,
	shared_system_state_t* system_state);
void update_sidebar_tasks_state(BITMAP* sidebar_box,
	task_state_t* task_states, const int task_states_size);


// ==================================================================
//                        AIRPLANE GRAPHIC
// ==================================================================
void get_triangle_coord(int xc, int yc, int radius, float angle, int* xs, int* ys);
void draw_triangle(BITMAP* bitmap, int xc, int yc, int radius, float angle,
	int color, int border_color);
void rotate_point(float* x, float* y, float xc, float yc, 
	float cos_angle, float sin_angle);
void convert_coord_to_display(int src_x, int src_y, int* dst_x, int* dst_y);
void draw_airplane(BITMAP* bitmap, const airplane_t* airplane);
void draw_point(BITMAP* bitmap, const waypoint_t* point, int color);
void draw_trail(BITMAP* bitmap, const cbuffer_t* trails, int n, int color);
int get_airplane_color(const airplane_t* airplane);

// ==================================================================
//                          KEYBOARD
// ==================================================================
struct textarea_attr_t {
	int width;					// width of the textarea
	int height;					// height of the textarea
	int color;					// font color
	int bg_color;				// background color
	int left_border;		// size of the left border;
	int top_border;			// size of the top border;
};

bool get_keycodes(char* scan, char* ascii);
int input_string(
		char* dest_str, int n_str,
		int x, int y, int color, int bg_color);
int textarea_prompt(char* dest_str, int n_str, int x, int y,
		const struct textarea_attr_t* attr);

#endif