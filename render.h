typedef unsigned long size_t;
typedef unsigned char uint8_t;
typedef unsigned int uint32_t;

#ifndef RD_NO_STDLIB
#include <stdlib.h>
#endif

#ifndef RD_ALLOC
#define RD_ALLOC(size) malloc(size)
#endif

typedef struct {
  size_t   width;
  size_t   height;
  uint32_t *pixels;
} rd_canvas;

typedef struct {
  uint8_t r;
  uint8_t g;
  uint8_t b;
  uint8_t a;
} rd_color;

#ifndef MT_IMPLEMENTATION
typedef struct {
  float x;
  float y;
} mt_Vec2;
#endif

typedef struct {
  float x, y, z;
} mt_Vec3;

typedef struct {
  float m[4][4];
} mt_Mat4;
 
static rd_color rd_grey = {.r=0x18, .g=0x18, .b=0x18, .a=0xFF};
static rd_color rd_red = {.r=0xFF, .g=0x00, .b=0x00, .a=0xFF};
static rd_color rd_white = {.r=0xFF, .g=0xFF, .b=0xFF, .a=0xFF};
static rd_color rd_green = {.r=0x00, .g=0xFF, .b=0x00, .a=0xFF};
static rd_color rd_blue = {.r=0x00, .g=0x00, .b=0xFF, .a=0xFF};

static uint32_t rd_color_to_uint32(rd_color col);
static rd_color uint32_to_rd_color(uint32_t rgba);
void rd_init_canvas(rd_canvas *c, size_t w, size_t h);
void rd_fill_background(rd_canvas *c, rd_color col);
void rd_draw_rect(rd_canvas *c, size_t w, size_t h, size_t x, size_t y, rd_color col);
void rd_draw_pixel(rd_canvas *c, size_t x, size_t y, rd_color color);
void rd_draw_triangle(rd_canvas *c, mt_Vec2 v1, mt_Vec2 v2, mt_Vec2 v3, rd_color col);
float rd_solve_y(mt_Vec2 a, mt_Vec2 b, float x);
int rd_ceil(float x);
int rd_floor(float x);
void rd_draw_line_vertical(rd_canvas *c, size_t start, size_t end, size_t x, size_t w, rd_color col);
void rd_draw_line_horizontal(rd_canvas *c, size_t start, size_t end, size_t y, size_t w, rd_color col);

#ifdef RD_IMPLEMENTATION
static inline uint32_t rd_color_to_uint32(rd_color col){
  return ((uint32_t)col.a << 24) | ((uint32_t)col.r << 16) |
         ((uint32_t)col.g << 8) | ((uint32_t)col.b);
}

static inline rd_color uint32_to_rd_color(uint32_t rgba){
  rd_color color = {.r=(rgba >> 16), .g=(rgba >> 8), .b=(rgba >> 0)};
  return color;
}

void rd_init_canvas(rd_canvas *c, size_t w, size_t h){
  c->width  = w;
  c->height = h;
  c->pixels = RD_ALLOC(sizeof(uint32_t) * h * w); 
}

void rd_fill_background(rd_canvas *c, rd_color col){
  uint32_t rgba = rd_color_to_uint32(col);
  for (size_t i=0; i< (c->height*c->width); ++i){
    c->pixels[i] = rgba;
  }
}

void rd_draw_rect(rd_canvas *c, size_t w, size_t h, size_t x, size_t y, rd_color col){
  uint32_t rgba = rd_color_to_uint32(col);

  if (x > c->width || y > c->height) return;
  
  size_t x_end = (x+w > c->width)  ? c->width:  x+w;
  size_t y_end = (y+h > c->height) ? c->height: y+h;

  for (size_t j=y; j<y_end; ++j){
    for (size_t i=x; i<x_end; ++i){
      c->pixels[j * c->width + i] = rgba;
    }  
  }
}

void rd_draw_pixel(rd_canvas *c, size_t x, size_t y, rd_color color){
  if (x >= c->width || y >= c->height){
    return;
  }
  c->pixels[y * c->width+x] = rd_color_to_uint32(color);
}

float rd_solve_y(mt_Vec2 a, mt_Vec2 b, float x){
  float slope = (b.y - a.y) / (b.x - a.x);
  return a.y + slope * (x - a.x); 
}

int rd_ceil(float x) {
  int cut = (int)x;
  if (x > (float)cut)
    return cut + 1;
  else return cut;
}


int rd_floor(float x) {
  int cut = (int)x;
  if (x < (float)cut) return cut - 1;
  else return cut;
}

void rd_draw_triangle(rd_canvas *c, mt_Vec2 v1, mt_Vec2 v2, mt_Vec2 v3, rd_color col){

  int xmin = rd_ceil(v1.x);
  int xmax = rd_floor(v3.x);

  mt_Vec2 mr = {.x = (v1.x + v3.x) * 0.5f}; 

  for (int x = xmin; x<=xmax; ++x){
    float hr;

    if (x<=mr.x){
      hr = rd_solve_y(v1, v2, (float)x);
    } else {
      hr = rd_solve_y(v3, v2, (float)x);
    }

    float ymin = v1.y;
    float ymax = hr;

    for (int y=rd_ceil(ymin); y<=rd_floor(ymax); ++y){
      rd_draw_pixel(c, x, y, col);
    }
  }
}

void rd_draw_line_vertical(rd_canvas *c, size_t start, size_t end, size_t x, size_t w, rd_color col){
  for (size_t y=start; y<end; ++y){
    for (size_t j=0; j<w; ++j){
      // printf("%zu %zu", x, y);
      rd_draw_pixel(c, x+j, y, col);
      }
    }
}

void rd_draw_line_horizontal(rd_canvas *c, size_t start, size_t end, size_t y, size_t w, rd_color col){
  for (size_t x=start; x<end; ++x){
    for (size_t j=0; j<w; ++j){
      // printf("%zu %zu", x, y);
      rd_draw_pixel(c, x, y+j, col);
      }
    }
}

mt_Vec3 mt_mat4_mul_vec3(mt_Mat4 m, mt_Vec3 v) {
    mt_Vec3 r;
    float w = m.m[3][0]*v.x + m.m[3][1]*v.y + m.m[3][2]*v.z + m.m[3][3];
    r.x = (m.m[0][0]*v.x + m.m[0][1]*v.y + m.m[0][2]*v.z + m.m[0][3]) / w;
    r.y = (m.m[1][0]*v.x + m.m[1][1]*v.y + m.m[1][2]*v.z + m.m[1][3]) / w;
    r.z = (m.m[2][0]*v.x + m.m[2][1]*v.y + m.m[2][2]*v.z + m.m[2][3]) / w;
    return r;
}

float powf(float x, float power){
  float result = 1.0f;
  for (size_t i=0; i<power; ++i){
    result*=x;
  }
  return result;
}

double tan(double x){
  return x / (powf(x, 3)/3);
}

mt_Mat4 mt_perspective(float fov, float aspect, float near, float far) {
    float f = 1.0f / tan(fov * 0.5f);
    mt_Mat4 m = {0};
    m.m[0][0] = f / aspect;
    m.m[1][1] = f;
    m.m[2][2] = (far + near) / (near - far);
    m.m[2][3] = (2 * far * near) / (near - far);
    m.m[3][2] = -1.0f;
    return m;
}

// Raylib's Camera can give the Matrix

#endif // RD_IMPLEMENTATION



