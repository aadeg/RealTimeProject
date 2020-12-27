#ifndef _GRAPHICS_H_
#define _GRAPHICS_H_

#include <stdbool.h>
#include <allegro.h>

#include "structs.h"


// ==================================================================
//                          STATUS BOX
// ==================================================================
BITMAP* create_status_box();
void blit_status_box(BITMAP* status_box);
void update_status_box(BITMAP* status_box, int counter);

// ==================================================================
//                        AIRPLANE GRAPHIC
// ==================================================================
void draw_triangle(BITMAP* bitmap, int xc, int yc, int radius, float angle,
	int color);
void rotate_point(float* x, float* y, float xc, float yc, 
	float cos_angle, float sin_angle);
void convert_coord_to_display(int src_x, int src_y, int* dst_x, int* dst_y);
void draw_airplane(const airplane_t* airplane, int color);
void draw_point(const waypoint_t* point, int color);
void draw_trail(const cbuffer_t* trails, int n, int color);

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