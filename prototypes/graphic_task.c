#include <allegro.h>
#include <stdbool.h>
#include <stdio.h>
#include <math.h>

#include "ptask.h"
#include "graphics.h"

// GRAPHIC CONSTANTS
#define SCREEN_WIDTH				1024
#define SCREEN_HEIGHT				720
#define BG_COLOR						0     // black
#define MAIN_COLOR					14		// yellow
#define STATUS_BOX_WIDTH		SCREEN_WIDTH - SCREEN_HEIGHT - 20
#define STATUS_BOX_HEIGHT		SCREEN_HEIGHT - 20
#define STATUS_BOX_X				SCREEN_HEIGHT + 10
#define STATUS_BOX_Y				10
#define STATUS_BOX_PADDING 	5


#define PERIOD_MS     20
#define PRIORITY      50

// GLOBAL VARIABLES

void init() {
  allegro_init();
  install_keyboard();

  set_color_depth(8);
  set_gfx_mode(GFX_AUTODETECT_WINDOWED, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 0);
  clear_to_color(screen, BG_COLOR);
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

void rotate_point(float* x, float* y, float cos_angle, float sin_angle) {
	float tmp_x = *x;
	*x = cos_angle * tmp_x - sin_angle * (*y);
	*y = sin_angle * tmp_x + cos_angle * (*y);
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
	rotate_point(&x1, &y1, cos_angle, sin_angle);
	rotate_point(&x2, &y2, cos_angle, sin_angle);
	rotate_point(&x3, &y3, cos_angle, sin_angle);
	triangle(bitmap,
			roundf(x1), roundf(y1),
			roundf(x2), roundf(y2),
			roundf(x3), roundf(y3), color);

}

void* graphic_task(void* arg) {
	struct task_info* task_info = (struct task_info*) arg;
  char str[100] = { 0 };
	int counter = 0;
	int x = 0;
	int y = 0;
	float angle = 0;
	BITMAP* status_box = create_status_box();

	task_set_activation(task_info);

	while (counter <= 1000) {
		x = rand() % SCREEN_WIDTH;
		y = rand() % SCREEN_HEIGHT;
		angle = (float) (rand() % 360) / (2.0 * M_PI);
		draw_triangle(screen, x, y, 5, angle, 11);

		textout_ex(status_box, font, str,
				STATUS_BOX_PADDING, STATUS_BOX_PADDING, BG_COLOR, BG_COLOR);
		if (counter < 1000)
			sprintf(str, "Messaggio di prova %d", counter);
		else
			sprintf(str, "Task terminato");
		textout_ex(status_box, font, str,
				STATUS_BOX_PADDING, STATUS_BOX_PADDING, MAIN_COLOR, BG_COLOR);
		blit_status_box(status_box);
		
		++counter;

		if (task_deadline_missed(task_info)) {
			fprintf(stderr, "Deadline miss\n");
			return NULL;
		}
		task_wait_for_activation(task_info);
	}

	printf("Exiting...\n");
	destroy_bitmap(status_box);
  return NULL;
}

int main() {
  int err = 0;
  struct task_info graphic_task_info;

  init();

  task_info_init(&graphic_task_info, 1, PERIOD_MS, PERIOD_MS, PRIORITY);
  err = task_create(&graphic_task_info, graphic_task);
  if (err) fprintf(stderr, "Errore while creating the task. Errno %d\n", err);

  err = task_join(&graphic_task_info, NULL);
  if (err) fprintf(stderr, "Errore while joining the task. Errno %d\n", err);

	readkey();
  allegro_exit();
  return 0;
}