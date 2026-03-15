#ifndef RD_NO_STDLIB
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#else
typedef unsigned long size_t;
typedef unsigned char uint8_t;
typedef unsigned int uint32_t;
#endif

#ifndef RD_ALLOC
#define RD_ALLOC(size) malloc(size)
#endif

typedef struct {
  size_t width;
  size_t height;
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

typedef enum {
  RD_NODE_RECT,
  RD_NODE_CIRCLE,
  RD_NODE_TRIANGLE,
  RD_NODE_GROUP
} rd_node_kind;

typedef struct rd_style {
  rd_color stroke;
  bool has_stroke;

  size_t stroke_width;
  bool has_stroke_width;

  rd_color fill;
  bool has_fill;

  float opacity;
  bool has_opacity;
} rd_style;

typedef struct rd_node {
  rd_node_kind Kind;
  mt_Vec2 position;
  mt_Vec2 scale;
  float rotation;
  const char *id;

  rd_color color;
  rd_style *style;

  union {
    struct {
      float w, h;
    } rect;
    struct {
      float r;
    } circle;
    struct {
      mt_Vec2 v0, v1, v2;
    } triangle;
  };

  struct rd_node *parent;
  struct rd_node *child;
  struct rd_node *next;
} rd_node;

static rd_color rd_grey = {.r = 0x18, .g = 0x18, .b = 0x18, .a = 0xFF};
static rd_color rd_red = {.r = 0xFF, .g = 0x00, .b = 0x00, .a = 0xFF};
static rd_color rd_white = {.r = 0xFF, .g = 0xFF, .b = 0xFF, .a = 0xFF};
static rd_color rd_green = {.r = 0x00, .g = 0xFF, .b = 0x00, .a = 0xFF};
static rd_color rd_blue = {.r = 0x00, .g = 0x00, .b = 0xFF, .a = 0xFF};

uint32_t rd_color_to_uint32(rd_color col);
static rd_color uint32_to_rd_color(uint32_t rgba);
void rd_init_canvas(rd_canvas *c, size_t w, size_t h);
void rd_fill_background(rd_canvas *c, rd_color col);
void rd_draw_rect(rd_canvas *c, size_t w, size_t h, size_t x, size_t y,
                  rd_color col);
void rd_draw_pixel(rd_canvas *c, size_t x, size_t y, rd_color color);
void rd_draw_triangle(rd_canvas *c, mt_Vec2 v1, mt_Vec2 v2, mt_Vec2 v3,
                      rd_color col);
float rd_solve_y(mt_Vec2 a, mt_Vec2 b, float x);
int rd_ceil(float x);
int rd_floor(float x);
void rd_draw_line_vertical(rd_canvas *c, size_t start, size_t end, size_t x,
                           size_t w, rd_color col);
void rd_draw_line_horizontal(rd_canvas *c, size_t start, size_t end, size_t y,
                             size_t w, rd_color col);
static void swap_vec2(mt_Vec2 *a, mt_Vec2 *b);
static void sort_by_y(mt_Vec2 *v0, mt_Vec2 *v1, mt_Vec2 *v2);
static float edge_interp_x(mt_Vec2 a, mt_Vec2 b, float y);
void rd_draw_ellipse(rd_canvas *c, int cx, int cy, float rx, float ry,
                     rd_color col);
void rd_draw_circle(rd_canvas *c, int cx, int cy, float r, rd_color col);
rd_node *rd_create_node(rd_node_kind kind);
void rd_append_child(rd_node *parent, rd_node *child);
void rd_render_node(rd_canvas *c, rd_node *node, mt_Vec2 parent_pos);
rd_style rd_resolve_style(rd_node *node);
void rd_style_set_fill(rd_node *node, rd_color c);
void rd_style_set_stroke(rd_node *node, rd_color c, size_t width);
void rd_style_set_opacity(rd_node *n, float opacity);
static rd_style *rd_ensure_style(rd_node *node);

#ifdef RD_IMPLEMENTATION
void rd_append_child(rd_node *parent, rd_node *child) {
  child->parent = parent;

  if (!parent->child) {
    parent->child = child;
  } else {
    rd_node *s = parent->child;
    while (s->next)
      s = s->next;
    s->next = child;
  }
}

rd_node *rd_create_node(rd_node_kind kind) {
  rd_node *n = RD_ALLOC(sizeof(rd_node));
  *n = (rd_node){0};
  n->Kind = kind;
  n->scale = (mt_Vec2){1, 1};
  return n;
}

void rd_render_node(rd_canvas *c, rd_node *node, mt_Vec2 parent_pos) {

  if (!node) return;

  rd_style style = rd_resolve_style(node);

  bool has_stroke       = style.has_stroke;
  size_t stroke_width   = style.stroke_width;
  bool has_stroke_width = style.has_stroke_width;
  rd_color fill         = style.fill;
  bool has_fill         = style.has_fill;
  float opacity         = style.opacity;
  bool has_opacity      = style.has_opacity;


  mt_Vec2 world_pos = {parent_pos.x + node->position.x,
                       parent_pos.y + node->position.y};

  switch (node->Kind) {

  case RD_NODE_RECT:
    if (has_fill) rd_draw_rect(c, node->rect.w, node->rect.h, world_pos.x, world_pos.y, fill);

    if (style.has_stroke) {
      size_t w = style.stroke_width;

      rd_draw_line_horizontal(c, world_pos.x, world_pos.x + node->rect.w,
                              world_pos.y, w, style.stroke);

      rd_draw_line_horizontal(c, world_pos.x, world_pos.x + node->rect.w,
                              world_pos.y + node->rect.h - w, w, style.stroke);

      rd_draw_line_vertical(c, world_pos.y, world_pos.y + node->rect.h,
                            world_pos.x, w, style.stroke);

      rd_draw_line_vertical(c, world_pos.y, world_pos.y + node->rect.h,
                            world_pos.x + node->rect.w - w, w, style.stroke);
    }
    break;

  case RD_NODE_CIRCLE:
    if (has_fill) rd_draw_circle(c, world_pos.x, world_pos.y, node->circle.r, fill);
    break;

  case RD_NODE_TRIANGLE: {
    mt_Vec2 v0 = {node->triangle.v0.x + world_pos.x,
                  node->triangle.v0.y + world_pos.y};
    mt_Vec2 v1 = {node->triangle.v1.x + world_pos.x,
                  node->triangle.v1.y + world_pos.y};
    mt_Vec2 v2 = {node->triangle.v2.x + world_pos.x,
                  node->triangle.v2.y + world_pos.y};

    if (has_fill) rd_draw_triangle(c, v0, v1, v2, fill);
    break;
  }

  case RD_NODE_GROUP:
    break;
  }

  rd_node *child = node->child;
  while (child) {
    rd_render_node(c, child, world_pos);
    child = child->next;
  }
}

void rd_draw_ellipse(rd_canvas *c, int cx, int cy, float rx, float ry,
                     rd_color col) {
  int minx = cx - rx;
  int miny = cy - ry;
  int maxx = cx + rx;
  int maxy = cy + ry;

  if (minx < 0)
    minx = 0;
  if (miny < 0)
    miny = 0;
  if (maxx >= c->width)
    maxx = c->width - 1;
  if (maxy >= c->height)
    maxy = c->height - 1;

  for (int y = miny; y <= maxy; y++) {
    for (int x = minx; x <= maxx; x++) {
      float dx = x - cx;
      float dy = y - cy;

      if ((dx * dx) / (rx * rx) + (dy * dy) / (ry * ry) <= 1.0f) {
        rd_draw_pixel(c, x, y, col);
      }
    }
  }
}

void rd_draw_circle(rd_canvas *c, int cx, int cy, float r, rd_color col) {
  rd_draw_ellipse(c, cx, cy, r, r, col);
}

static void swap_vec2(mt_Vec2 *a, mt_Vec2 *b) {
  mt_Vec2 tmp = *a;
  *a = *b;
  *b = tmp;
}

static void sort_by_y(mt_Vec2 *v0, mt_Vec2 *v1, mt_Vec2 *v2) {
  if (v0->y > v1->y)
    swap_vec2(v0, v1);
  if (v0->y > v2->y)
    swap_vec2(v0, v2);
  if (v1->y > v2->y)
    swap_vec2(v1, v2);
}

static float edge_interp_x(mt_Vec2 a, mt_Vec2 b, float y) {
  if (a.y == b.y)
    return a.x;
  float t = (y - a.y) / (b.y - a.y);
  return a.x + t * (b.x - a.x);
}

uint32_t rd_color_to_uint32(rd_color col) {
  return ((uint32_t)col.a << 24) | ((uint32_t)col.r << 16) |
         ((uint32_t)col.g << 8) | ((uint32_t)col.b);
}

static inline rd_color uint32_to_rd_color(uint32_t rgba) {
  rd_color color = {.r = (rgba >> 16), .g = (rgba >> 8), .b = (rgba >> 0)};
  return color;
}

void rd_init_canvas(rd_canvas *c, size_t w, size_t h) {
  c->width = w;
  c->height = h;
  c->pixels = RD_ALLOC(sizeof(uint32_t) * h * w);
}

void rd_fill_background(rd_canvas *c, rd_color col) {
  uint32_t rgba = rd_color_to_uint32(col);
  for (size_t i = 0; i < (c->height * c->width); ++i) {
    c->pixels[i] = rgba;
  }
}

void rd_draw_rect(rd_canvas *c, size_t w, size_t h, size_t x, size_t y,
                  rd_color col) {
  uint32_t rgba = rd_color_to_uint32(col);

  if (x > c->width || y > c->height)
    return;

  size_t x_end = (x + w > c->width) ? c->width : x + w;
  size_t y_end = (y + h > c->height) ? c->height : y + h;

  for (size_t j = y; j < y_end; ++j) {
    for (size_t i = x; i < x_end; ++i) {
      c->pixels[j * c->width + i] = rgba;
    }
  }
}

void rd_draw_pixel(rd_canvas *c, size_t x, size_t y, rd_color color) {
  if (x >= c->width || y >= c->height) {
    return;
  }
  c->pixels[y * c->width + x] = rd_color_to_uint32(color);
}

float rd_solve_y(mt_Vec2 a, mt_Vec2 b, float x) {
  float slope = (b.y - a.y) / (b.x - a.x);
  return a.y + slope * (x - a.x);
}

int rd_ceil(float x) {
  int cut = (int)x;
  if (x > (float)cut)
    return cut + 1;
  else
    return cut;
}

int rd_floor(float x) {
  int cut = (int)x;
  if (x < (float)cut)
    return cut - 1;
  else
    return cut;
}

void rd_draw_triangle(rd_canvas *c, mt_Vec2 v0, mt_Vec2 v1, mt_Vec2 v2,
                      rd_color col) {
  sort_by_y(&v0, &v1, &v2);

  if (v0.y == v2.y)
    return;

  int y_start = rd_ceil(v0.y);
  int y_end = rd_floor(v2.y);

  for (int y = y_start; y <= y_end; ++y) {

    if (y < v1.y) {
      float x1 = edge_interp_x(v0, v2, (float)y);
      float x2 = edge_interp_x(v0, v1, (float)y);

      if (x1 > x2) {
        float tmp = x1;
        x1 = x2;
        x2 = tmp;
      }

      int x_start = rd_ceil(x1);
      int x_end = rd_floor(x2);

      for (int x = x_start; x <= x_end; ++x) {
        rd_draw_pixel(c, x, y, col);
      }

    } else {
      float x1 = edge_interp_x(v0, v2, (float)y);
      float x2 = edge_interp_x(v1, v2, (float)y);

      if (x1 > x2) {
        float tmp = x1;
        x1 = x2;
        x2 = tmp;
      }

      int x_start = rd_ceil(x1);
      int x_end = rd_floor(x2);

      for (int x = x_start; x <= x_end; ++x) {
        rd_draw_pixel(c, x, y, col);
      }
    }
  }
}

void rd_draw_line_vertical(rd_canvas *c, size_t start, size_t end, size_t x,
                           size_t w, rd_color col) {
  for (size_t y = start; y < end; ++y) {
    for (size_t j = 0; j < w; ++j) {
      rd_draw_pixel(c, x + j, y, col);
    }
  }
}

void rd_draw_line_horizontal(rd_canvas *c, size_t start, size_t end, size_t y,
                             size_t w, rd_color col) {
  for (size_t x = start; x < end; ++x) {
    for (size_t j = 0; j < w; ++j) {
      rd_draw_pixel(c, x, y + j, col);
    }
  }
}

mt_Vec3 mt_mat4_mul_vec3(mt_Mat4 m, mt_Vec3 v) {
  mt_Vec3 r;
  float w = m.m[3][0] * v.x + m.m[3][1] * v.y + m.m[3][2] * v.z + m.m[3][3];
  r.x = (m.m[0][0] * v.x + m.m[0][1] * v.y + m.m[0][2] * v.z + m.m[0][3]) / w;
  r.y = (m.m[1][0] * v.x + m.m[1][1] * v.y + m.m[1][2] * v.z + m.m[1][3]) / w;
  r.z = (m.m[2][0] * v.x + m.m[2][1] * v.y + m.m[2][2] * v.z + m.m[2][3]) / w;
  return r;
}

float powf(float x, float power) {
  float result = 1.0f;
  for (size_t i = 0; i < power; ++i) {
    result *= x;
  }
  return result;
}

double tan(double x) { return x / (powf(x, 3) / 3); }

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
static rd_style *rd_ensure_style(rd_node *node) {
  if (!node->style) {
    node->style = RD_ALLOC(sizeof(rd_style));
    *node->style = (rd_style){0};
  }
  return node->style;
}

rd_style rd_resolve_style(rd_node *node) {
  rd_style result = {0};

  if (node->parent)
    result = rd_resolve_style(node->parent);

  if (node->style) {
    if (node->style->has_fill) {
      result.fill = node->style->fill;
      result.has_fill = true;
    }

    if (node->style->has_stroke) {
      result.stroke = node->style->stroke;
      result.has_stroke = true;
    }

    if (node->style->has_stroke_width) {
      result.stroke_width = node->style->stroke_width;
      result.has_stroke_width = true;
    }

    if (node->style->has_opacity) {
      result.opacity = node->style->opacity;
      result.has_opacity = true;
    }
  }

  return result;
}

void rd_style_set_fill(rd_node *node, rd_color c) {
  rd_style *s = rd_ensure_style(node);
  s->fill = c;
  s->has_fill = true;
}

void rd_style_set_stroke(rd_node *node, rd_color c, size_t width) {
  rd_style *s = rd_ensure_style(node);
  s->stroke = c;
  s->stroke_width = width;
  s->has_stroke = true;
  s->has_stroke_width = true;
}

void rd_style_set_opacity(rd_node *node, float opacity) {
  rd_style *s = rd_ensure_style(node);
  s->opacity = opacity;
  s->has_opacity = 1;
}
#endif // RD_IMPLEMENTATION
