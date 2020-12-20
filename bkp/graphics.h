#ifndef _GRAPHICS_H_
#define _GRAPHICS_H_

#include <stdbool.h>
#include <allegro.h>

// ==================================================================
//                          STATUS BOX
// ==================================================================
BITMAP* create_status_box();
void blit_status_box(BITMAP* status_box);
void draw_triangle(BITMAP* bitmap, int xc, int yc, int radius, float angle,
	int color);

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