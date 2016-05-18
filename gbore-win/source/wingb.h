#pragma once

typedef 
struct wingb_rect_s {
    uint32_t w, h;
} wingb_rect_s;

typedef  uint32_t wingb_px_t; //RGBA

typedef 
struct wingb_display_s {


    WNDCLASS wc;
    HWND     wnd_h;
    HDC      DC_h;
    HGLRC    RC_h;
    MSG      winmsg;
    uint32_t framebuf[GB_SCREEN_W][GB_SCREEN_H];
    
    wingb_rect_s win_size;
    wingb_rect_s screen_res;



} wingb_display_s;

extern wingb_display_s g_Win;


void 
wingb_display_init(
    WNDPROC   wndProc,
    HINSTANCE hInstance, 
    wchar_t *title, 
    unsigned char fullscreen, 
    int width, 
    int height
);

void wingb_framebuffer_init();
void wingb_framebuffer_update();


bool wingb_redraw(bool is_sync);
void wingb_swap_buffers(void);
void wingb_display_destroy(void);

LRESULT CALLBACK wingb_DefaultWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
