#include <stddef.h>
#include <stdint.h>
#include <unistd.h>
#define MT_IMPLEMENTATION // TODO: FIX THIS
#define RD_IMPLEMENTATION 
#include "../renderfull.h"
#include "../mathx.h"
#include <raylib.h>

#define WIDTH 800
#define HEIGHT 600

#define FPS 200 

int main(void){
  rd_canvas canva = {0};
  rd_init_canvas(&canva, WIDTH, HEIGHT);
  InitWindow(WIDTH, HEIGHT, "hello from raylib");

  Image img = {.data=canva.pixels, .width=WIDTH, .height=HEIGHT, .mipmaps=1, .format=PIXELFORMAT_UNCOMPRESSED_R8G8B8A8};
  Texture2D tex = LoadTextureFromImage(img);
  mt_Vec2 rec1 = {.x=200, .y=200};


  while (!WindowShouldClose()){
    float dt = GetFrameTime();
    // SetTargetFPS(FPS);
            
    mt_Vec2transformP(&rec1, 40*dt, 80*dt);
    
    rd_fill_background(&canva, rd_white);
    rd_draw_rect(&canva, 160, 80, 0, 0, rd_blue);
    rd_draw_rect(&canva, 120, 80, rec1.x, rec1.y, rd_red);
    rd_draw_rect(&canva, 40, 80, 160, 240, rd_green);

    BeginDrawing();    
    {
      UpdateTexture(tex, canva.pixels);
      DrawTexture(tex, 0, 0, WHITE);
      DrawFPS(700, 10);
    }
    EndDrawing();
  }
  CloseWindow();
  return 0;
}
