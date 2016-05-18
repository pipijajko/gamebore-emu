#include <windows.h>
#include <stdbool.h>
#include <stdint.h>
#include <gl/gl.h>
//gb_gpu_CHR_s#include "timer.h"
#include "display.h"
#include "wingb.h"
wingb_display_s g_Win;

void
wingb_framebuffer_init()
{

    glViewport(0, 0, GB_SCREEN_W, GB_SCREEN_H);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, GB_SCREEN_W, 0, GB_SCREEN_H, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    for (int y = 0; y < GB_SCREEN_W; y++) {
        for (int x = 0; x < GB_SCREEN_H; x++) {
            /*if (x % 2 == 0) {
                g_Win.framebuf[y][x][0] = g_Win.framebuf[y][x][1] = g_Win.framebuf[y][x][2] = 0;
            }
            else {
                g_Win.framebuf[y][x][0] = g_Win.framebuf[y][x][1] = g_Win.framebuf[y][x][2] = 0xff;
            }*/
        }
    }




    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,
                GB_SCREEN_W, GB_SCREEN_H, 0,
                GL_RGB, GL_UNSIGNED_BYTE, 0);

    // Set up the texture
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

    // Enable textures
    glEnable(GL_TEXTURE_2D);




}



void
wingb_framebuffer_pix(uint32_t x, uint32_t y, uint32_t rgba) {

}

void
wingb_framebuffer_update(){
    

    //for (int y = 0; y < GB_SCREEN_W; y++) 
    //    for (int x = 0; x < GB_SCREEN_H; x++) 
    //        if (x%2) g_Win.framebuf[y][x][0] = g_Win.framebuf[y][x][1] = g_Win.framebuf[y][x][2] = 0xF0;

    //glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 
    //                GB_SCREEN_W, GB_SCREEN_H,
    //                GL_RGB, GL_UNSIGNED_BYTE, (GLvoid*)g_Win.framebuf);
    glTexSubImage2D(GL_TEXTURE_2D, 0, GL_RGB,
                     GB_SCREEN_W, GB_SCREEN_H, 0,
                     GL_RGB, GL_UNSIGNED_BYTE, (GLvoid*)g_Win.framebuf);
    glBegin(GL_QUADS);
    glTexCoord2d(0.0, 0.0);		glVertex2d(0.0, 0.0);
    glTexCoord2d(1.0, 0.0); 	glVertex2d(g_Win.win_size.w, 0.0);
    glTexCoord2d(1.0, 1.0); 	glVertex2d(g_Win.win_size.w, g_Win.win_size.h);
    glTexCoord2d(0.0, 1.0); 	glVertex2d(0.0, g_Win.win_size.h);
    glEnd();
}


// All the ugly OS API stuff is here.


void wingb_display_init(
    WNDPROC   wndProc,
    HINSTANCE hInstance,
    wchar_t *title,
    unsigned char fullscreen,
    int width,
    int height
){
    
    memset(&g_Win, 0, sizeof(wingb_display_s));
    int32_t screen_w = GetSystemMetrics(SM_CXSCREEN);
    int32_t screen_h = GetSystemMetrics(SM_CYSCREEN);

    wchar_t const * const 
        cls_name = L"WINGBClass";
    
    LDFS_InitTimer();
    
    g_Win.wc.lpfnWndProc     = wndProc;
    g_Win.wc.style           = CS_OWNDC;
    g_Win.wc.cbClsExtra      = 0;
    g_Win.wc.cbWndExtra      = 0;
    g_Win.wc.hInstance       = hInstance;
    g_Win.wc.hIcon           = LoadIcon(NULL, IDI_APPLICATION);
    g_Win.wc.hCursor         = LoadCursor(NULL, IDC_ARROW);
    g_Win.wc.hbrBackground   = (HBRUSH)GetStockObject(BLACK_BRUSH);
    g_Win.wc.lpszMenuName    = NULL;
    g_Win.wc.lpszClassName   = cls_name;
    RegisterClass(&g_Win.wc);

    if (fullscreen) {
        width = GetSystemMetrics(SM_CXSCREEN);
        height = GetSystemMetrics(SM_CYSCREEN);

        g_Win.wnd_h = CreateWindow(cls_name,
                                   title, 
                                   WS_EX_TOPMOST | WS_POPUP, 
                                   0, 0,  
                                   screen_w, screen_h, 
                                   NULL, 
                                   NULL, 
                                   hInstance, 
                                   NULL);

        ShowWindow(g_Win.wnd_h, SW_MAXIMIZE);
    }
    else {
        if (!width)  width  = 640;
        if (!height) height = 480;
        int32_t pos_x = screen_w / 2 - width / 2;
        int32_t pos_y = screen_h / 2 - height / 2;

        g_Win.wnd_h = CreateWindow(cls_name, 
                                   title, 
                                   WS_CAPTION | WS_POPUPWINDOW, 
                                   pos_x, pos_y,
                                   width, height, 
                                   NULL, 
                                   NULL, 
                                   hInstance, 
                                   NULL);

        ShowWindow(g_Win.wnd_h, SW_SHOWNORMAL);
    }

    //
    // Enable OpenGL
    //
    PIXELFORMATDESCRIPTOR pxformat = {
        .nSize = sizeof(PIXELFORMATDESCRIPTOR),
        .nVersion = 1,
        .dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
        .iPixelType = PFD_TYPE_RGBA,
        .cColorBits = 24,
        .cDepthBits = 16,
        .iLayerType = PFD_MAIN_PLANE,
    };
    //
    // Get Drawing Canvas and set pixel format for display
    //
    g_Win.DC_h = GetDC(g_Win.wnd_h);
    SetPixelFormat(g_Win.DC_h,
                   ChoosePixelFormat(g_Win.DC_h, &pxformat),
                   &pxformat);

    g_Win.RC_h = wglCreateContext(g_Win.DC_h);

    wglMakeCurrent(g_Win.DC_h, g_Win.RC_h);

    g_Win.screen_res = (wingb_rect_s){ screen_w, screen_h};
    g_Win.win_size   = (wingb_rect_s){ width, height };

}



void
wingb_display_destroy()
{
    if (g_Win.wnd_h) {

        wglMakeCurrent(NULL, NULL);
        if (g_Win.RC_h) wglDeleteContext(g_Win.RC_h);
        if (g_Win.DC_h) ReleaseDC(g_Win.wnd_h, g_Win.DC_h);
        if (g_Win.wnd_h) DestroyWindow(g_Win.wnd_h);
        g_Win.RC_h = NULL;
        g_Win.DC_h = NULL;
        g_Win.wnd_h = NULL;
    }
}


bool
wingb_redraw(bool is_sync) {

    if(is_sync) LDFS_MaintainFramerate();

    if (PeekMessage(&g_Win.winmsg, NULL, 0, 0, PM_REMOVE)) {
        if (g_Win.winmsg.message == WM_QUIT) {
            return FALSE;
        }
        else {
            TranslateMessage(&g_Win.winmsg);
            DispatchMessage(&g_Win.winmsg);
        }
    }

    return TRUE;
}


void wingb_swap_buffers() { SwapBuffers(g_Win.DC_h); }



LRESULT CALLBACK wingb_DefaultWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE:
        return 0;

    case WM_CLOSE:
        PostQuitMessage(0);
        return 0;

    case WM_DESTROY:
        return 0;

    case WM_KEYDOWN:
        switch (wParam) {
        case VK_ESCAPE:
            PostQuitMessage(0);
            return 0;
        }
        return 0;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
}
