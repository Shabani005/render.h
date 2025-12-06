#include <stddef.h>
#include <stdint.h>
#include <unistd.h>

#define RD_NATIVE
#define RD_IMPLEMENTATION
#define MT_IMPLEMENTATION
#define NB_IMPLEMENTATION

#include "../mathx.h"
#include "../renderfull.h"

#define WIDTH 800
#define HEIGHT 600

int main(void) {
  rd_canvas canva = {0};
  rd_init_canvas(&canva, WIDTH, HEIGHT);
  rd_window win = rd_init_window(&canva, WIDTH, HEIGHT, "X11 example");

  mt_Vec2 rec1 = {.x = 200, .y = 200};
  mt_Vec2 t1 = {.x = 200, .y = 50};
  mt_Vec2 t2 = {.x = 400, .y = 200};
  mt_Vec2 t3 = {.x = 600, .y = 50};

  float last_time = rd_time();
  bool running = true;

  while (running) {
    running = rd_poll(&win);
    float t = rd_time();
    float dt = (float)(t - last_time);
    last_time = t;

    mt_Vec2transformP(&rec1, 40 * dt, 80 * dt);
    rd_fill_background(&canva, rd_white);
    rd_draw_rect(&canva, 160, 80, 0, 0, rd_blue);
    rd_draw_rect(&canva, 120, 80, rec1.x, rec1.y, rd_red);
    rd_draw_rect(&canva, 40, 80, 160, 240, rd_green);
    rd_draw_triangle(&canva, t1, t2, t3, rd_red);
    rd_draw_circle(&canva, 300, 300, 50, rd_grey);
    rd_draw(&win, &canva);
    rd_sleep_frame();
  }

  rd_close_window(&win);
  return 0;
}
