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

} gbdisp_context_s;



// Internal Functions declarations

static gbdisp_context_s g_context;
void internal_idle_evt(void);
void internal_display_evt(void);
void internal_input_evt(void);
void internal_reshape_window(GLsizei w, GLsizei h);
void internal_texture_update(void);



void internal_timer(int t) {
    t;
    glutPostRedisplay();
    glutTimerFunc(REFRESH_INTERVAL, internal_timer, 0);
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

    d->screen_buffer[0] = (gb_color_s) { .RGBA = 0x0 };
    //TODO:Make platform agnostic
    timeBeginPeriod(1); // Make Windows system timers spin like crazy

    //
    // Initialize GLUT
    //
    size_t const win_h = h * mod;
    size_t const win_w = w * mod;

    glutInit(&cfg.argc, cfg.argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);

    glutInitWindowSize(win_w, win_h);
    glutInitWindowPosition(320, 320); //TODO: set screen center
    glutCreateWindow(win_name);

    glutIdleFunc(internal_idle_evt);
    glutDisplayFunc(internal_display_evt);
    glutReshapeFunc(internal_reshape_window);
    //glutKeyboardFunc()
    //glutTimerFunc(REFRESH_INTERVAL, internal_timer, 1);
    
    // Create a texture 
    glTexImage2D(GL_TEXTURE_2D, 0, 3, w , h, 0, GL_RGBA, GL_UNSIGNED_BYTE, d->screen_buffer);

    // Set up the texture
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

    glEnable(GL_TEXTURE_2D);


    d->cfg = cfg;
    d->is_enabled = true; 
    *handle = d->self = (gbdisplay_h) { .unused = d };

    return 0;
}

void gbdisp_run(gbdisplay_h handle) {
    UNUSED(handle);
    glutMainLoop();
}
    


void gbdisp_putpixel(gbdisplay_h handle, uint8_t x, uint8_t y, gb_color_s c) {
    UNUSED(handle); //for future implementations

    uint32_t const scr_w = g_context.cfg.width;
    StopIf(y * scr_w + x > g_context.screen_bytes, abort(), 
           "screen overflow!")
    
    g_context.screen_ready = false; //dirty
    //
    //Calculate Pixel location in linear buffer
    //
    g_context.screen_buffer[y * scr_w + x] = c;
}


void gbdisp_buffer_ready(gbdisplay_h handle) {
    UNUSED(handle); //for future implementations

    g_context.screen_ready = true;
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
    //if (g_context.cfg.callbacks.OnIdle) {
    //    g_context.cfg.callbacks.OnIdle(g_context.self, g_context.cfg.callback_context);
    //}
    // Draw pixels to texture
    internal_texture_update();// will this tear - no double buff
    // Swap buffers!
    glutSwapBuffers();
}



void internal_idle_evt(void) {

    if (g_context.cfg.callbacks.OnIdle) {
        g_context.cfg.callbacks.OnIdle(g_context.self, g_context.cfg.callback_context);
    }
    if (g_context.screen_ready) {
        internal_texture_update();
        g_context.screen_ready = false;
        glutPostRedisplay();
    }

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