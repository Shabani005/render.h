#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#define MX_IMPLEMENTATION
#include "mathx.h"

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

static rd_color rd_grey = {.r=0x18, .g=0x18, .b=0x18, .a=0xFF};
static rd_color rd_red = {.r=0xFF, .g=0x00, .b=0x00, .a=0xFF};
static rd_color rd_white = {.r=0xFF, .g=0xFF, .b=0xFF, .a=0xFF};
static rd_color rd_green = {.r=0x00, .g=0xFF, .b=0x00, .a=0xFF};
static rd_color rd_blue = {.r=0x00, .g=0x00, .b=0xFF, .a=0xFF};

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
  c->pixels = malloc(sizeof(uint32_t) * h * w); 
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

void rd_canvas_to_ppm(rd_canvas *c, const char *filename){
  FILE *f = fopen(filename, "wb");
  
  fprintf(f, "P6\n%zu %zu\n255\n", c->width, c->height);

  for (size_t i=0; i<c->height*c->width; ++i){
    rd_color sep = uint32_to_rd_color(c->pixels[i]);
    uint8_t r = sep.r;
    uint8_t g = sep.g;
    uint8_t b = sep.b;

    fwrite(&r, 1, 1, f);
    fwrite(&g, 1, 1, f);
    fwrite(&b, 1, 1, f);
  }
  fclose(f);
}

void rd_canvas_to_terminal(){
  
}



void canvas_to_term(rd_canvas *c){
  printf("\n\033[H\033[J");
  for (size_t j=0; j<c->height; j+=(int)c->height/20){ // the larger the canvas the worse the approximation is tested using 200x200
    for (size_t i=0; i<c->width; i+=(int)c->width/64){
      rd_color col = uint32_to_rd_color(c->pixels[j * c->width + i]);
      if (col.r ==  rd_white.r && col.g == rd_white.g && col.b == rd_white.b){
        printf("\033[37m||\033[0m");
      } else if (col.r ==  rd_green.r && col.g == rd_green.g && col.b == rd_green.b) {
        printf("\033[32m||\033[0m");
      } else if (col.r ==  rd_red.r && col.g == rd_red.g && col.b == rd_red.b) {
        printf("\033[31m||\033[0m");
      } else if (col.r ==  rd_blue.r && col.g == rd_blue.g && col.b == rd_blue.b) {
        printf("\033[34m||\033[0m");
      } else {
        printf("\033[90m||\033[0m");
      }
    }
    printf("\n");
  }
  printf("\n");
  fflush(stdout);
}


#define FPS 30

int main(void){
  rd_canvas canva = {0};
  rd_init_canvas(&canva, 200, 100);

  float dt = (float)1/FPS; 
  Vec2 rec1 = {.x=50, .y=50};
    
  while (true){
    usleep(1000 * 1000 * dt); 
    Vec2transformP(&rec1, 10*dt, 20*dt);
    rd_fill_background(&canva, rd_white);
    rd_draw_rect(&canva, 40, 20, 0, 0, rd_blue);
    rd_draw_rect(&canva, 30, 20, rec1.x, rec1.y, rd_red);
    rd_draw_rect(&canva, 10, 20, 40, 60, rd_green);
    canvas_to_term(&canva);
  }
  // canvas_to_term(&canva);
  // rd_canvas_to_ppm(&canva, "image.ppm");
  return 0;
}
