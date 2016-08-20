/*
Copyright 2016 Maciej Kurczewski

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/
#include <assert.h>

#include <Windows.h>
#include <gl/GL.h>
#include <GL/glut.h>


#include "gamebore.h"

//NOTE: GLUT can only be used from the main thread.

#define REFRESH_INTERVAL (1000/GB_FPS_LIMIT)


typedef struct gbdisp_context_s {
    gbdisplay_h         self;

    gbdisp_config_s     cfg;
    size_t              screen_bytes;
    gb_color_s*         screen_buffer;
    bool                screen_ready;
    //uint8_t             ready_screen_n; //index of screen that is ready for drawing
    bool is_enabled; //changing this will stop and do a cleanup
    
    bool is_initialized;
    

    // for timer start/stop operations
    uint64_t qpc_frequency;
    uint64_t qpc_start; 

} gbdisp_context_s;



// Internal Functions declarations

static gbdisp_context_s g_context;
//void internal_idle_evt(void);
void internal_main_timer_evt(int);
void internal_display_evt(void);
void internal_input_evt(unsigned char key, int x, int y);
void internal_input_release_evt(unsigned char key, int x, int y);
void internal_specialinput_evt(int key, int x, int y);
void internal_specialinput_release_evt(int key, int x, int y);
void internal_reshape_window(GLsizei w, GLsizei h);
void internal_texture_update(void);




#define CLK_TO_MS(c) ((c) / 1000)
typedef uint64_t clk_us_t; //microsecond clock

void internal_clock_start() {

    LARGE_INTEGER pc;
    assert(0ULL == g_context.qpc_start); // make sure we dont call start twice

    QueryPerformanceCounter(&pc);
    g_context.qpc_start = pc.QuadPart;
}

clk_us_t internal_clock_stop() {

    LARGE_INTEGER pc;
    QueryPerformanceCounter(&pc);
    //
    // To guard against loss-of-precision, we convert
    // to microseconds *before* dividing by ticks-per-second.
    //
    uint64_t diff = (pc.QuadPart - g_context.qpc_start);
    diff         *= 1000000;
    diff         /= g_context.qpc_frequency;
    
    g_context.qpc_start = 0; //reset timer context
    return diff;
}

errno_t 
gbdisp_init(gbdisp_config_s cfg, gbdisplay_h *handle)
{
    StopIf(!cfg.height || !cfg.width, return EINVAL, "Invalid display size");
    StopIf(!cfg.size_modifier, return EINVAL, "screen size modifier must be > 0");
    StopIf(cfg.size_modifier > 64, return EINVAL, "screen size modifier is > 32");
    StopIf(g_context.is_initialized, abort(), "fatal, already initialized glut display");
    
    g_context.is_initialized = true;
    
    uint32_t const h = cfg.height;
    uint32_t const w = cfg.width;
    uint8_t  const mod = cfg.size_modifier; //screen size modifier, fixed for now

    char *const win_name = (cfg.window_name) ? (cfg.window_name) : ("Default");
    
    //
    // Create context and initialize screen buffer
    //
    gbdisp_context_s * const d = &g_context;
    d->screen_buffer = calloc(h * w, sizeof(gb_color_s));
    d->screen_bytes  = h * w * sizeof(gb_color_s);

    memset(d->screen_buffer, 0xFF, (h * w * sizeof(gb_color_s)));

    //TODO:Make platform agnostic
    timeBeginPeriod(1); // Make Windows system timers spin like crazy

    //
    // Initialize GLUT
    //
    size_t const win_h = h * mod;
    size_t const win_w = w * mod;

    glutInit(&cfg.argc, cfg.argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA| GLUT_ALPHA);
    /*glEnable(GL_BLEND);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBlendFunc(GL_ONE_MINUS_DST_ALPHA, GL_DST_ALPHA);
    glClearColor(0.0, 0.0, 0.0, 0.0);*/

    glutInitWindowSize(win_w, win_h);
    glutInitWindowPosition(320, 320); //TODO: set screen center
    glutCreateWindow(win_name);



    //glutIdleFunc(internal_idle_evt);
    glutDisplayFunc(internal_display_evt);
    glutReshapeFunc(internal_reshape_window);
    glutKeyboardFunc(internal_input_evt);
    glutKeyboardUpFunc(internal_input_release_evt);

    glutSpecialFunc(internal_specialinput_evt);
    glutSpecialUpFunc(internal_specialinput_release_evt);

    glutSetKeyRepeat(GLUT_KEY_REPEAT_OFF);

    glutTimerFunc(REFRESH_INTERVAL, internal_main_timer_evt, 0);
    
    // Create a texture 
    glTexImage2D(GL_TEXTURE_2D, 0, 3, w , h, 0, GL_RGBA, GL_UNSIGNED_BYTE, d->screen_buffer);

    // Set up the texture
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

    glEnable(GL_TEXTURE_2D);


    LARGE_INTEGER frq;
    QueryPerformanceFrequency(&frq); //QPF will never fail on OS >= Win XP, see msdn
    d->qpc_frequency = frq.QuadPart;
    d->cfg = cfg;
    d->is_enabled = true; 
    *handle = d->self = (gbdisplay_h) { .unused = d };

    return 0;
}

void gbdisp_run(gbdisplay_h handle) 
{
    UNUSED(handle);
    glutMainLoop();
}
    


void gbdisp_putpixel(gbdisplay_h handle, uint8_t x, uint8_t y, gb_color_s c) 
{
    UNUSED(handle); //for future implementations

    uint32_t const scr_w = g_context.cfg.width;
    StopIf(y * scr_w + x > g_context.screen_bytes, abort(),
           "screen overflow!")

    g_context.screen_ready = false; //dirty
    if (!c.A) return; //homebrew transparency :F
    //
    //Calculate Pixel location in linear buffer
    //
    g_context.screen_buffer[y * scr_w + x] = c;
}


void gbdisp_buffer_set_ready(gbdisplay_h handle) 
{
    UNUSED(handle); //for future implementations

    g_context.screen_ready = true;
}

bool gbdisp_is_buffer_ready(gbdisplay_h handle) 
{
    UNUSED(handle); //for future implementations
    return g_context.screen_ready;
}


void gbdisp_stop(gbdisplay_h handle) {
    UNUSED(handle); //for future implementations

    g_context.is_enabled = false;
}


void internal_texture_update(void)
{
    // Update Texture
    size_t const scr_h = g_context.cfg.height;
    size_t const scr_w = g_context.cfg.width;
    size_t const win_h = scr_h * g_context.cfg.size_modifier;
    size_t const win_w = scr_w * g_context.cfg.size_modifier;


    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, scr_w, scr_h, GL_RGBA, GL_UNSIGNED_BYTE,
                    g_context.screen_buffer);

    glBegin(GL_QUADS);
    glTexCoord2d(0.0, 0.0);		glVertex2d(0.0, 0.0);
    glTexCoord2d(1.0, 0.0); 	glVertex2d(win_w, 0.0);
    glTexCoord2d(1.0, 1.0); 	glVertex2d(win_w, win_h);
    glTexCoord2d(0.0, 1.0); 	glVertex2d(0.0, win_h);
    glEnd();

}

void internal_display_evt(void) {
    // Clear framebuffer
    //glClear(GL_COLOR_BUFFER_BIT);
    // Draw pixels to texture
    internal_texture_update();// will this tear - no double buff
    glutSwapBuffers();
}






void internal_main_timer_evt(int t) {
    UNUSED(t);
    
    internal_clock_start();

    if (g_context.cfg.callbacks.OnPrepareFrame) {
        g_context.cfg.callbacks.OnPrepareFrame(g_context.self, g_context.cfg.callback_context);
    }

    if (g_context.screen_ready) {
        internal_texture_update();
        g_context.screen_ready = false;
        glutPostRedisplay();
    }
    clk_us_t const elapsed_time = internal_clock_stop();

    //
    // Schedule this function to render next frame 
    //
    int const wait_time = REFRESH_INTERVAL - (int)CLK_TO_MS(elapsed_time);

    if (wait_time < 0) {
        fprintf(stderr, "rendering took:%lluus, wait_time:%d ms\n", elapsed_time, wait_time);
    }

    glutTimerFunc(wait_time < 0 ? 0 : wait_time, internal_main_timer_evt, wait_time);
}

void internal_reshape_window(GLsizei w, GLsizei h)
{
    UNUSED(w, h);

    // Update Texture
    size_t const scr_h = g_context.cfg.height;
    size_t const scr_w = g_context.cfg.width;
    size_t const win_h = scr_h * g_context.cfg.size_modifier;
    size_t const win_w = scr_w * g_context.cfg.size_modifier;


    glClearColor(0.0f, 0.0f, 0.5f, 0.0f);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, win_w, win_h, 0);
    glMatrixMode(GL_MODELVIEW);
    glViewport(0, 0, win_w, win_h);


    glutReshapeWindow(win_w, win_h);
}

//
// oh how i hate you, freeglut
// how about adding 10 more input callbacks? huh? HUUUH?
//
//the key mapping somewhat arbitrary and hardcoded, but will do for now
__forceinline
gb_INPUT_key_e internal_specialinput_map_key(int key) {
    switch (key) {
    case GLUT_KEY_LEFT:  return gb_INPUT_left;
    case GLUT_KEY_RIGHT: return gb_INPUT_right;
    case GLUT_KEY_UP:    return gb_INPUT_up;
    case GLUT_KEY_DOWN:  return gb_INPUT_down;
    case GLUT_KEY_HOME:  return gb_INPUT_start;
    case GLUT_KEY_END:   return gb_INPUT_select;
    default:             return gb_INPUT_none;
    }
}

__forceinline
gb_INPUT_key_e internal_input_map_key(unsigned char key) {
    
    switch (key) {
    case 'Q':
        gbdbg_mute_print(g_GB.dbg, true); //hax
        return gb_INPUT_none;
    case 'q':
        gbdbg_mute_print(g_GB.dbg, false);
        return gb_INPUT_none;

    case 'z':
    case 'Z':            return gb_INPUT_A;
    case 'x':
    case 'X':            return gb_INPUT_B;
    default:             return gb_INPUT_none;

    }
}

__forceinline 
void internal_on_key_input(gb_INPUT_key_e const mapped_key, bool is_press) {
    if (mapped_key != gb_INPUT_none && g_context.cfg.callbacks.OnKeyInput) {
        g_context.cfg.callbacks.OnKeyInput(mapped_key, is_press);
    }
}

void internal_specialinput_evt(int key, int x, int y) {
    UNUSED(x, y);
    internal_on_key_input(internal_specialinput_map_key(key), true);
}

void internal_specialinput_release_evt(int key, int x, int y) {
    UNUSED(x, y);
    internal_on_key_input(internal_specialinput_map_key(key), false);
}

void internal_input_evt(unsigned char key, int x, int y) {
    UNUSED(x, y);
    internal_on_key_input(internal_input_map_key(key), true);
}

void internal_input_release_evt(unsigned char key, int x, int y) {
    UNUSED(x, y);
    internal_on_key_input(internal_input_map_key(key), false);
}

