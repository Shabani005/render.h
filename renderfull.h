#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>

#ifndef _WIN32
#include <termios.h>
#include <sys/select.h>
#include <unistd.h>
#endif

#ifdef _WIN32
#include <Windows.h>
#endif 

#define FPS 60

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

#ifndef _WIN32
#ifdef RD_NATIVE
#include <X11/X.h>
#include <X11/Xutil.h>
#include <X11/Xlib.h>

typedef struct {
  Display *dsp;
  Window win;
  GC gc;
  XImage *img;
  XVisualInfo vinfo;
  int depth;
} rd_window;
#endif
#endif

#ifndef MT_IMPLEMENTATION
typedef struct {
  float x;
  float y;
} mt_Vec2;
#endif

#ifndef RD_ALLOC
#include <stdlib.h>
#define RD_ALLOC(size) malloc(size)
#endif

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
void rd_canvas_to_ppm(rd_canvas *c, const char *filename);
uint8_t rd_poll_key_terminal();
void rd_canvas_to_terminal(rd_canvas *c);
void rd_draw_pixel(rd_canvas *c, size_t x, size_t y, rd_color color);
void rd_draw_triangle(rd_canvas *c, mt_Vec2 v1, mt_Vec2 v2, mt_Vec2 v3, rd_color col);
float rd_solve_y(mt_Vec2 a, mt_Vec2 b, float x);
int rd_ceil(float x);
int rd_floor(float x);
void rd_draw_ellipse(rd_canvas *c, int cx, int cy, float rx, float ry, rd_color col);
void rd_draw_circle(rd_canvas *c, int cx, int cy, float r, rd_color col);
void rd_draw_line_vertical(rd_canvas *c, size_t start, size_t end, size_t x, size_t w, rd_color col);
void rd_draw_line_horizontal(rd_canvas *c, size_t start, size_t end, size_t y, size_t w, rd_color col);


// WINDOWING STUFF

#ifdef RD_NATIVE
rd_window rd_init_window(rd_canvas *canva, int width, int height, const char *title);
bool rd_poll(rd_window *w);
void rd_draw(rd_window *w, rd_canvas *canva);
void rd_sleep_frame();
void rd_close_window(rd_window *w);
double rd_time(void);
#endif


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

#ifndef _WIN32
uint8_t rd_poll_key_terminal() {
    struct termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    int ch = -1;
    struct timeval tv = {0L, 0L};
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);

    int ret = select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv);
    if (ret > 0 && FD_ISSET(STDIN_FILENO, &fds)) {
        ch = getchar();
    }

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return ch;  
}
#endif

void rd_canvas_to_terminal(rd_canvas *c){
  printf("\n\033[H\033[J");
  for (size_t j=0; j<c->height; j+=(int)c->height/20){ // these two values are "magical" they may not be the best
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

void rd_draw_ellipse(rd_canvas *c, int cx, int cy, float rx, float ry, rd_color col) {
  int minx = cx - rx;
  int miny = cy - ry;
  int maxx = cx + rx;
  int maxy = cy + ry;

  if (minx < 0) minx = 0;
  if (miny < 0) miny = 0;
  if (maxx >= c->width)  maxx = c->width - 1;
  if (maxy >= c->height) maxy = c->height - 1;

  for (int y = miny; y <= maxy; y++) {
    for (int x = minx; x <= maxx; x++) {
      float dx = x - cx;
      float dy = y - cy;

      if ((dx*dx)/(rx*rx) + (dy*dy)/(ry*ry) <= 1.0f) {
                rd_draw_pixel(c, x, y, col);
              }
        }
    }
}

void rd_draw_circle(rd_canvas *c, int  cx, int cy, float r, rd_color col){
  rd_draw_ellipse(c, cx, cy, r, r, col);
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

#ifndef _WIN32
#ifdef RD_NATIVE
rd_window rd_init_window(rd_canvas *canva, int width, int height, const char *title) {
  rd_window w = {0};
  w.dsp = XOpenDisplay(NULL);
  int screen = DefaultScreen(w.dsp);
  if (!XMatchVisualInfo(w.dsp, screen, 32, TrueColor, &w.vinfo)) {
    XMatchVisualInfo(w.dsp, screen, 24, TrueColor, &w.vinfo);
  }
  w.depth = w.vinfo.depth;
  Colormap cmap =
      XCreateColormap(w.dsp, RootWindow(w.dsp, screen), w.vinfo.visual,
                      AllocNone);
  XSetWindowAttributes attrs;
  attrs.colormap = cmap;
  attrs.border_pixel = 0;
  attrs.background_pixel = 0;
  attrs.event_mask = ExposureMask | KeyPressMask | StructureNotifyMask;
  w.win = XCreateWindow(w.dsp, RootWindow(w.dsp, screen), 0, 0, width, height,
                        0, w.vinfo.depth, InputOutput, w.vinfo.visual,
                        CWColormap | CWBorderPixel | CWBackPixel | CWEventMask,
                        &attrs);
  XStoreName(w.dsp, w.win, title);
  XMapWindow(w.dsp, w.win);
  w.gc = XCreateGC(w.dsp, w.win, 0, NULL);
  w.img = XCreateImage(w.dsp, w.vinfo.visual, w.vinfo.depth, ZPixmap, 0,
                       (char *)canva->pixels, width, height, 32, 0);
  return w;
}

bool rd_poll(rd_window *w) {
  while (XPending(w->dsp)) {
    XEvent event;
    XNextEvent(w->dsp, &event);
    if (event.type == KeyPress || event.type == DestroyNotify) return false;
  }
  return true;
}

void rd_draw(rd_window *w, rd_canvas *canva) {
  XPutImage(w->dsp, w->win, w->gc, w->img, 0, 0, 0, 0, canva->width,
            canva->height);
  XFlush(w->dsp);
}

void rd_sleep_frame() {
  struct timespec ts;
  ts.tv_sec = 0;
  ts.tv_nsec = 1000000000 / FPS;
  nanosleep(&ts, NULL);
}

void rd_close_window(rd_window *w) {
  XDestroyImage(w->img);
  XDestroyWindow(w->dsp, w->win);
  XCloseDisplay(w->dsp);
}
#endif // RD_NATIVE
#endif

#ifdef _WIN32

typedef struct {
    HWND hwnd;
    HDC hdc;
    BITMAPINFO bmi;
    bool running;
} rd_window;

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_DESTROY) {
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

rd_window rd_init_window(rd_canvas* canva, int width, int height, const char* title) {
    rd_window w = { 0 };

    HINSTANCE hInstance = GetModuleHandle(NULL);
    const wchar_t CLASS_NAME[] = L"Win32CanvasClass";

    WNDCLASSW wc = { 0 };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    RegisterClassW(&wc);

    // Convert const char* title → wchar_t
    wchar_t wtitle[256];
    MultiByteToWideChar(CP_UTF8, 0, title, -1, wtitle, 256);

    w.hwnd = CreateWindowExW(
        0,
        CLASS_NAME,
        wtitle, 
        WS_OVERLAPPEDWINDOW ^ WS_THICKFRAME,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        width,
        height,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    w.hdc = GetDC(w.hwnd);

    ZeroMemory(&w.bmi, sizeof(BITMAPINFO));
    w.bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    w.bmi.bmiHeader.biWidth = (LONG)canva->width;
    w.bmi.bmiHeader.biHeight = -(LONG)canva->height;
    w.bmi.bmiHeader.biPlanes = 1;
    w.bmi.bmiHeader.biBitCount = 32;
    w.bmi.bmiHeader.biCompression = BI_RGB;

    ShowWindow(w.hwnd, SW_SHOW);
    UpdateWindow(w.hwnd);
    w.running = true;
    return w;
}

bool rd_poll(rd_window* w) {
    MSG msg;
    while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) {
            w->running = false;
            return false;
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return w->running;
}

void rd_draw(rd_window* w, rd_canvas* canva) {
    StretchDIBits(
        w->hdc,
        0, 0, canva->width, canva->height,
        0, 0, canva->width, canva->height,
        canva->pixels,
        &w->bmi,
        DIB_RGB_COLORS,
        SRCCOPY
    );
}

void rd_sleep_frame() {
    Sleep(1000 / FPS);
}

void rd_close_window(rd_window* w) {
    ReleaseDC(w->hwnd, w->hdc);
    DestroyWindow(w->hwnd);
}
#endif // _WIN32

#ifdef _WIN32
#include <Windows.h>

double rd_time(void) {
    static LARGE_INTEGER freq;
    static BOOL freqSet = FALSE;
    if (!freqSet) {
        QueryPerformanceFrequency(&freq);
        freqSet = TRUE;
    }
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    return (double)counter.QuadPart / (double)freq.QuadPart;
}
#else
#include <time.h>
double rd_time(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}
#endif
#endif // RD_IMPLEMENTATION

