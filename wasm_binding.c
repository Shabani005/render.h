typedef unsigned long size_t;
typedef unsigned char uint8_t;
typedef unsigned int uint32_t;

static unsigned char heap[64 * 1024 * 1024];
static size_t heap_pos = 0;

void *my_malloc(size_t size) {
  if (heap_pos + size > sizeof(heap)) return 0;
  void *p = &heap[heap_pos];
  heap_pos += size;
  return p;
}

#define RD_NO_STDLIB
#define RD_IMPLEMENTATION

#undef RD_ALLOC
#define RD_ALLOC(size) my_malloc(size) 

#include "render.h"


__attribute__((export_name("make_canvas")))
rd_canvas* make_canvas(size_t w, size_t h) {
  rd_canvas* c = RD_ALLOC(sizeof(rd_canvas));
  rd_init_canvas(c, w, h);
  return c;
}

__attribute__((export_name("fill_background")))
void fill_background(rd_canvas *c, uint32_t color){
  rd_fill_background(c, rd_red); // TODO: unhardcode color
}

// __atribute__((export_name("js_draw_rect")))
// void js_draw_rect(rd_canvas *c);

__attribute__((export_name("draw_rect")))
void draw_rect(rd_canvas* c, size_t width, size_t height, size_t x, size_t y, uint32_t color) {
  rd_draw_rect(c, width, height, x, y, rd_green);
}

__attribute__((export_name("draw_triangle")))
void draw_triangle(rd_canvas *c, float x1, float y1, float x2, float y2, float x3, float y3, uint32_t color){
  mt_Vec2 t1 = {.x=x1, .y=y1};
  mt_Vec2 t2 = {.x=x2, .y=y2};
  mt_Vec2 t3 = {.x=x3, .y=y3};
  rd_draw_triangle(c, t1, t2, t3, uint32_to_rd_color(color));
}

__attribute__((export_name("canvas_pixels")))
uint32_t* canvas_pixels(rd_canvas* c) {
  return c->pixels;
}

__attribute__((export_name("canvas_width")))
size_t canvas_width(rd_canvas* c) {
  return c->width;
}

__attribute__((export_name("canvas_height")))
size_t canvas_height(rd_canvas* c) {
  return c->height;
}
