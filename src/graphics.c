#include <stdio.h>
#include <math.h>

#include "graphics.h"
#include "consts.h"

#define CHAR_SPACING 8
#define CHAR_CURSOR '_'

#define SQRT_3		1.732050808

static BITMAP* bg_bitmap = NULL;
static int sidebar_box_airplanes_y = 0;
static int sidebar_box_runways_y[] = { 0, 0 };



int cbuffer_next_index(cbuffer_t* buffer);

// ==================================================================
//                            MAIN BOX
// ==================================================================
BITMAP* create_main_box() {
	BITMAP* main_box = create_bitmap(MAIN_BOX_WIDTH, MAIN_BOX_HEIGHT);
	clear_main_box(main_box);
	rect(main_box, 0, 0, MAIN_BOX_WIDTH - 1, MAIN_BOX_HEIGHT - 1,
		MAIN_COLOR);
	return main_box;
}

void clear_main_box(BITMAP* main_box) {
	if (bg_bitmap == NULL)
		bg_bitmap = load_bitmap("assets/background.bmp", NULL);
	blit(bg_bitmap, main_box, 0, 0, 0, 0, MAIN_BOX_WIDTH, MAIN_BOX_HEIGHT);
}

void blit_main_box(BITMAP* main_box) {
	rect(main_box, 0, 0, MAIN_BOX_WIDTH - 1, MAIN_BOX_HEIGHT - 1,
		MAIN_COLOR);
	blit(main_box, screen, 0, 0, MAIN_BOX_X, MAIN_BOX_Y,
		MAIN_BOX_WIDTH, MAIN_BOX_HEIGHT);
}

// ==================================================================
//                         SIDEBAR BOX
// ==================================================================
BITMAP* create_sidebar_box() {
	BITMAP* sidebar_box = create_bitmap(SIDEBAR_BOX_WIDTH, SIDEBAR_BOX_HEIGHT);
	int y = SIDEBAR_BOX_PADDING;

	clear_to_color(sidebar_box, BG_COLOR);
	rect(sidebar_box, 0, 0, SIDEBAR_BOX_WIDTH - 1, SIDEBAR_BOX_HEIGHT - 1,
		MAIN_COLOR);

	textout_ex(sidebar_box, font, "KEYBOARD COMMANDS",
		SIDEBAR_BOX_PADDING, y, MAIN_COLOR, BG_COLOR);
	y += SIDEBAR_BOX_VSPACE + SIDEBAR_BOX_PADDING;
	textout_ex(sidebar_box, font, "I: spawn an Inbound airplane",
		SIDEBAR_BOX_PADDING, y, MAIN_COLOR, BG_COLOR);
	y += SIDEBAR_BOX_VSPACE;
	textout_ex(sidebar_box, font, "O: spawn an Outbound airplane",
		SIDEBAR_BOX_PADDING, y, MAIN_COLOR, BG_COLOR);
	y += SIDEBAR_BOX_VSPACE;
	textout_ex(sidebar_box, font, "T: show / hide trails",
		SIDEBAR_BOX_PADDING, y, MAIN_COLOR, BG_COLOR);
	y += SIDEBAR_BOX_VSPACE;
	textout_ex(sidebar_box, font, "W: show / hide next waypoint",
		SIDEBAR_BOX_PADDING, y, MAIN_COLOR, BG_COLOR);
	y += SIDEBAR_BOX_VSPACE + SIDEBAR_BOX_PADDING;
	line(sidebar_box, 0, y, SIDEBAR_BOX_WIDTH, y, MAIN_COLOR);
	y += SIDEBAR_BOX_PADDING;
	textout_ex(sidebar_box, font, "SYSTEM STATE",
		SIDEBAR_BOX_PADDING, y, MAIN_COLOR, BG_COLOR);
	y += SIDEBAR_BOX_VSPACE + SIDEBAR_BOX_PADDING;
	textout_ex(sidebar_box, font, "Airplanes: ",
		SIDEBAR_BOX_PADDING, y, MAIN_COLOR, BG_COLOR);
	sidebar_box_airplanes_y = y;

	y += SIDEBAR_BOX_VSPACE;
	textout_ex(sidebar_box, font, "Runway 1:",
		SIDEBAR_BOX_PADDING, y, MAIN_COLOR, BG_COLOR);
	sidebar_box_runways_y[0] = y;

	y += SIDEBAR_BOX_VSPACE;
	textout_ex(sidebar_box, font, "Runway 2:",
		SIDEBAR_BOX_PADDING, y, MAIN_COLOR, BG_COLOR);
	sidebar_box_runways_y[1] = y;

	return sidebar_box;
}

void blit_sidebar_box(BITMAP* sidebar_box) {
	blit(sidebar_box, screen, 0, 0, SIDEBAR_BOX_X, SIDEBAR_BOX_Y,
		SIDEBAR_BOX_WIDTH, SIDEBAR_BOX_HEIGHT);
}

void update_sidebar_box(BITMAP* sidebar_box, const system_state_t* system_state) {
	char str[40] = { 0 };
	int i = 0;

	rect(sidebar_box, SIDEBAR_BOX_PADDING, sidebar_box_airplanes_y, 
		SIDEBAR_BOX_WIDTH - SIDEBAR_BOX_PADDING,
		SIDEBAR_BOX_HEIGHT - SIDEBAR_BOX_PADDING,
		BG_COLOR);

	sprintf(str, "Airplanes:  %02d / %02d", system_state->n_airplanes, MAX_AIRPLANE);
	textout_ex(sidebar_box, font, str,
		SIDEBAR_BOX_PADDING, sidebar_box_airplanes_y, MAIN_COLOR, BG_COLOR);
	for (i = 0; i < N_RUNWAYS; ++i) {
		if (system_state->is_runway_free[i])
			sprintf(str, "Runway %d:   %s", i + 1, "free");
		else
			sprintf(str, "Runway %d:   %s", i + 1, "busy");
		textout_ex(sidebar_box, font, str,
			SIDEBAR_BOX_PADDING, sidebar_box_runways_y[i], MAIN_COLOR, BG_COLOR);
	}
}

// ==================================================================
//                        AIRPLANE GRAPHIC
// ==================================================================
void get_triangle_coord(int xc, int yc, int radius, float angle, int* xs, int* ys) {
	float sin_angle = sinf(angle);
	float cos_angle = cosf(angle);
	float x1 = SQRT_3 * radius / 2.0 + xc;
	float y1 = ((float) radius) / 2.0 + yc;
	float x2 = xc;
	float y2 = yc - radius;
	float x3 =  -SQRT_3 * radius / 2.0 + xc;
	float y3 = y1;
	rotate_point(&x1, &y1, xc, yc, cos_angle, sin_angle);
	rotate_point(&x2, &y2, xc, yc, cos_angle, sin_angle);
	rotate_point(&x3, &y3, xc, yc, cos_angle, sin_angle);
	xs[0] = roundf(x1);
	xs[1] = roundf(x2);
	xs[2] = roundf(x3);
	ys[0] = roundf(y1);
	ys[1] = roundf(y2);
	ys[2] = roundf(y3);
}

void draw_triangle(BITMAP* bitmap, int xc, int yc, int radius, float angle,
		int color, int border_color) {
	int xs[3];
	int ys[3];
	get_triangle_coord(xc, yc, radius, angle, xs, ys);
	triangle(bitmap, xs[0], ys[0], xs[1], ys[1], xs[2], ys[2], color);
}

void rotate_point(float* x, float* y, float xc, float yc, 
		float cos_angle, float sin_angle) {
	float tmp_x;
	*x -= xc;
	*y -= yc;
	tmp_x = *x;

	*x = cos_angle * tmp_x + sin_angle * (*y);
	*y = -sin_angle * tmp_x + cos_angle * (*y);
	
	*x += xc;
	*y += yc;
}

void convert_coord_to_display(int src_x, int src_y, int* dst_x, int* dst_y) {
	*dst_x = src_x + SCREEN_HEIGHT/2.0;
	*dst_y = -src_y + SCREEN_HEIGHT/2.0;
}

void draw_airplane(BITMAP* bitmap, const airplane_t* airplane) {
	int x = 0;
	int y = 0;
	float angle = (airplane->angle - M_PI_2);
	int airplane_color = get_airplane_color(airplane);

	convert_coord_to_display(airplane->x, airplane->y, &x, &y);
	draw_triangle(bitmap, x, y, AIRPLANE_SIZE, angle, airplane_color, AIRPLANE_BORDER_COLOR);
}

void draw_point(BITMAP* bitmap, const waypoint_t* point, int color) {
	int x = 0;
	int y = 0;
	convert_coord_to_display(point->x, point->y, &x, &y);
	circlefill(bitmap, x, y, 2, color);
}


void draw_trail(BITMAP* bitmap, const cbuffer_t* trails, int n, int color) {
	int i = 0;
	int idx = 0;
	
	if (n > TRAIL_BUFFER_LENGTH) n = TRAIL_BUFFER_LENGTH;
	
	for (i = 0; i < n; ++i) {
		idx = (trails->top + TRAIL_BUFFER_LENGTH - i) % TRAIL_BUFFER_LENGTH;
		putpixel(bitmap, trails->points[idx].x, trails->points[idx].y, color);
	}
}

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