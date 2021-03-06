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


#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <assert.h>
#include <windows.h>
#include <gl/gl.h>
#include <gl/GLU.h>

#include "gamebore.h"

//Initialize Stopif macro
char error_mode = '\0';
FILE *error_log = NULL;


gbdisp_event_evt gb_disp_redraw;
gbdisp_event_evt gb_disp_prepare_frame;

void usage() {
    fprintf(stdout,
            "\n\nUsage:\n"
            "\tgamebore.exe romfile.gb\n"
            "Controls:\n"
                "\tArrows\t: D-PAD\n"
                "\tA \t: Z,z\n"
                "\tB \t: X,x\n"
                "\tStart \t: HOME\n"
                "\tSelect\t: END\n"
            "Debug Controls:\n"
                "\tCPU Tracing ON\t: q\n"
                "\tCPU Tracing OF\t: Q\n\n\n"
    );


}

int main(int argc, char *argv[])
{
    uint8_t *ROM_file      = NULL;
    size_t   ROM_size = 0;
    FILE    *fp       = NULL;
    size_t   fsize    = 0;
    errno_t  e        = 0;

    for (int i = 0; i < argc; i++) {
        printf("argv[%d]:%s\n", i, argv[i]);
    }

    if (argc > 1) {
        e = fopen_s(&fp, argv[1], "rb");
        if (e || !fp) goto cleanup_generic;

        // determine file size
        fseek(fp, 0L, SEEK_END);
        fsize = ftell(fp);
        rewind(fp);

        //TODO: add real checks
        assert(NULL == ROM_file);
        assert(fsize < 1024 * 1024); //it's gameboy after all

        ROM_file = (uint8_t*)malloc(fsize);
        if (!ROM_file) goto cleanup_generic;

        ROM_size = fread_s(ROM_file, fsize, 1, fsize, fp);
        assert(ROM_size == fsize);
    } else {
        usage();
        goto cleanup_generic;
    }


    /////////////////////////////////////////////
    // Initialize MMU, CPU and Cartdridge data
    /////////////////////////////////////////////
    gb_initialize(ROM_file, ROM_size);

    free(ROM_file); //gb_initialize has made a copy, dispose

    //
    // Initialize program window 
    // Seed main program loop with IdleCallback
    //
    gbdisp_config_s configuration =
    {
        argc, argv,
        .height = GB_SCREEN_H,
        .width = GB_SCREEN_W,
        .size_modifier = 4,
        .window_name = "GameBore",
        .callbacks.OnPrepareFrame = gb_disp_prepare_frame,
        .callbacks.OnRedraw = gb_disp_redraw,
        .callbacks.OnKeyInput = gb_INPUT_press_key,
        .callback_context = NULL,
    };
    gbdisplay_h display ;
    e = gbdisp_init(configuration, &display);
    if (e) goto cleanup_generic;

    gbdisp_run(display);

    
    //gbdisp_destroy()

cleanup_generic:
    if (e) {
        fprintf(stderr, "initialization failed e:%d,errno:%d\n", e, errno);
        usage();
    }
    if (ROM_file) free(ROM_file);
    if (fp) fclose(fp);
    gb_destroy();

    return 0;
}


void gb_disp_prepare_frame(gbdisplay_h disp, void* context) {

    UNUSED(context);

    while (!gbdisp_is_buffer_ready(disp)) {
        gb_machine_step(disp);
    }
}


void gb_disp_redraw(gbdisplay_h disp, void* context) 
{
    UNUSED(context, disp);
}

