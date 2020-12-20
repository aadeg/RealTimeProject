#include <allegro.h>
#include <stdbool.h>
#include <stdio.h>

#include "ptask.h"
#include "graphics.h"

// CONSTANTS
#define SCREEN_WIDTH  640
#define SCREEN_HEIGHT 480
#define BG_COLOR      0     // black

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
    }
  } while (scan != KEY_ESC);

  printf("Exiting...\n");
  return NULL;
}

int main() {
  int err = 0;
  struct task_info input_task_info;

  init();

  task_info_init(&input_task_info, 1, PERIOD_MS, PERIOD_MS, PRIORITY);
  err = task_create(&input_task_info, input_task);
  if (err) fprintf(stderr, "Errore while creating the task. Errno %d\n", err);

  err = task_join(&input_task_info, NULL);
  if (err) fprintf(stderr, "Errore while joining the task. Errno %d\n", err);
  allegro_exit();
  return 0;
}