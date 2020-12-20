#include "graphics.h"
#include "utils.h"

#include <stdio.h>
#include "consts.h"

#define CHAR_SPACING 8
#define CHAR_CURSOR '_'

#define SQRT_3		1.732050808

// ==================================================================
//                          STATUS BOX
// ==================================================================
BITMAP* create_status_box() {
	BITMAP* status_box = create_bitmap(STATUS_BOX_WIDTH, STATUS_BOX_HEIGHT);
	clear_to_color(status_box, BG_COLOR);
	rect(status_box, 0, 0, STATUS_BOX_WIDTH - 1, STATUS_BOX_HEIGHT - 1,
		MAIN_COLOR);
	return status_box;
}

void blit_status_box(BITMAP* status_box) {
	blit(status_box, screen, 0, 0, STATUS_BOX_X, STATUS_BOX_Y,
		STATUS_BOX_WIDTH, STATUS_BOX_HEIGHT);
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

// ==================================================================
//                          KEYBOARD
// ==================================================================
void print_char(char c, int x, int y, int color, int bg_color,
		bool with_cursor) {
	char str[3] = { '\0', '\0', '\0' };

	str[0] = c;
	if (with_cursor)
		str[1] = CHAR_CURSOR;

	textout_ex(screen, font, str, x, y, color, bg_color);
}

void clear_char(char c, int x, int y, int color, int bg_color,
		bool with_cursor) {
	char str[3] = { '\0', '\0', '\0' };

	str[0] = c;
	if (with_cursor)
		str[1] = CHAR_CURSOR;
	
	textout_ex(screen, font, str, x, y, bg_color, bg_color);

	str[0] = CHAR_CURSOR;
	str[1] = '\0';
	textout_ex(screen, font, str, x, y, color, bg_color);
}

void blink_cursor(int x, int y, int color, int bg_color) {
	static int counter = 0;
	static bool visible = true;

	char str[2] = {CHAR_CURSOR, '\0'};

	if (counter == 0 && visible) {
		textout_ex(screen, font, str, x, y, bg_color, bg_color);
		visible = false;
	} else if (counter == 0 && !visible) {
		textout_ex(screen, font, str, x, y, color, bg_color);
		visible = true;
	}

	if (counter == 0) {
		printf("here\n");
		counter = 50000000;
	}

	--counter;
}

// get_keycodes puts the scan number and the ascii code of the
// key pressed, if any.
// scan and ascii are valid only when true is returned.
// return true if a key is pressed, otherwise false.
bool get_keycodes(char* scan, char* ascii) {
	int key;
	
	if (!keypressed())
		return false;
	
	key = readkey();		// block until a key is pressed
	*ascii = (char) key & 0xFF;
	*scan = (char) (key >> 8);
	return true;
}

// input_string reads a string written via the keyboard
//
// dest_str:	result of the input
// n_str:     max dimension of the input string (null char excluded)
// x:					x-coord of the textbox area
// y:					y-coord of the textbox area
// color:			color of the text
// bg_color:	color of the background
//
// return SUCCESS or ERROR_GENERIC
int input_string(char* dest_str, int n_str,
		int x, int y, int color, int bg_color) {
	// Checking the arguments
	if (n_str <= 0) return ERROR_GENERIC;

	bool got_key = false;								// true if get_keycodes read a new key
	char ascii, scan;										// returned by get_keycodes
	int i = 0;													// next char index
	bool with_cursor = true;

	print_char(CHAR_CURSOR, x, y, color, bg_color, false);

	do {
		got_key = get_keycodes(&scan, &ascii);
		
		if (got_key && scan == KEY_BACKSPACE && i > 0) {
			// clearing the last character
			with_cursor = i < n_str;
			x -= CHAR_SPACING;
			clear_char(dest_str[i - 1], x, y, color, bg_color, with_cursor);
			--i;
			
		} else if (got_key && i < n_str && 
		           scan != KEY_ENTER && scan != KEY_BACKSPACE) {
			// printing the new character
			with_cursor = (i + 1) < n_str;
			print_char(ascii, x, y, color, bg_color, with_cursor);
			x += CHAR_SPACING;
			dest_str[i] = ascii;
			++i;
		
		} else if (i < n_str) {
			blink_cursor(x, y, color, bg_color);
		}

	} while (scan != KEY_ENTER);

	return SUCCESS;
}

int textarea_prompt(char* dest_str, int n_str, int x, int y,
		const struct textarea_attr_t* attr) {
  rectfill(screen, x, y, x + attr->width, y + attr->height, attr->bg_color);
	return input_string(dest_str, n_str,
			x + attr->left_border, y + attr->top_border, attr->color, attr->bg_color);
}